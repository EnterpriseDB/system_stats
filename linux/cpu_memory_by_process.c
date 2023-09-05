/*------------------------------------------------------------------------
 * cpu_memory_by_process.c
 *              CPU and memory usage by process id or name
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"

#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/sysinfo.h>

#define READ_PROCESS_CPU_USAGE_FIRST_SAMPLE     1
#define READ_PROCESS_CPU_USAGE_SECOND_SAMPLE    2

static long long unsigned int total_cpu_usage_1 = 0;
static long long unsigned int total_cpu_usage_2 = 0;

/* structure used to store the data for each process */
typedef struct node
{
	long long unsigned int pid;
	long long unsigned int process_cpu_sample_1;
	long long unsigned int process_cpu_sample_2;
	long long unsigned int rss_memory;
	unsigned long long process_up_since_seconds;
	char name[MAXPGPATH];
	struct node * next;
} node_t;

node_t *head = NULL;
node_t *prev = NULL;
node_t *iter = NULL;

/* Function used to get number of processor count */
int ReadTotalProcessors(void);
/* Function used to get total physical RAM available on system */
uint64 ReadTotalPhysicalMemory(void);
/* Function used to read total cpu usage for each process */
uint64 ReadTotalCPUUsage(void);
/* Function used to read total memory usage for each process */
void ReadCPUMemoryUsage(int sample);

void ReadCPUMemoryByProcess(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* Read the total number of processors of the system */
int ReadTotalProcessors()
{
	int num_processors;
	num_processors = sysconf(_SC_NPROCESSORS_ONLN);
	return num_processors;
}


/* Read the total physical memory available in the system */
uint64 ReadTotalPhysicalMemory()
{
	FILE       *memory_file;
	char       *line_buf = NULL;
	size_t     line_buf_size = 0;
	ssize_t    line_size;
	uint64     total_memory = 0;

	/* Open the file required to read all the memory information */
	memory_file = fopen(MEMORY_FILE_NAME, "r");

	if (!memory_file)
	{
		char memory_file_name[MAXPGPATH];
		snprintf(memory_file_name, MAXPGPATH, "%s", MEMORY_FILE_NAME);
		ereport(DEBUG1,
				(errcode_for_file_access(),
				errmsg("can not open file %s for reading memory statistics",
					memory_file_name)));
		return 0;
	}

	/* Get the first line of the file. */
	line_size = getline(&line_buf, &line_buf_size, memory_file);

	/* Loop through until we are done with the file. */
	while (line_size >= 0)
	{
		/* Read the total memory of the system */
		if (strstr(line_buf, "MemTotal") != NULL)
		{
			total_memory = ConvertToBytes(line_buf);
			break;
		}

		/* Free the allocated line buffer */
		if (line_buf != NULL)
		{
			free(line_buf);
			line_buf = NULL;
		}

		/* Get the next line */
		line_size = getline(&line_buf, &line_buf_size, memory_file);
	}

	/* Free the allocated line buffer */
	if (line_buf != NULL)
	{
		free(line_buf);
		line_buf = NULL;
	}

	/* Close the file now that we are done with it */
	fclose(memory_file);

	return total_memory;
}

/* Read the total CPU usage */
uint64 ReadTotalCPUUsage()
{
	FILE       *cpu_stats_file;
	char       *line_buf = NULL;
	size_t     line_buf_size = 0;
	ssize_t    line_size;
	char       cpu_name[MAXPGPATH];
	uint64     total_cpu_time = 0;
	uint64     usermode_normal_process = 0;
	uint64     usermode_niced_process = 0;
	uint64     kernelmode_process = 0;
	uint64     idle_mode = 0;
	uint64     io_completion = 0;
	const char *scan_fmt = "%s %llu %llu %llu %llu %llu";

	memset(cpu_name, 0, MAXPGPATH);

	cpu_stats_file = fopen(CPU_USAGE_STATS_FILENAME, "r");

	if (!cpu_stats_file)
	{
		char cpu_stats_file_name[MAXPGPATH];
		snprintf(cpu_stats_file_name, MAXPGPATH, "%s", CPU_USAGE_STATS_FILENAME);

		ereport(DEBUG1,
				(errcode_for_file_access(),
					errmsg("can not open file %s for reading cpu usage statistics",
						cpu_stats_file_name)));
		return 0;
	}

	/* Get the first line of the file. */
	line_size = getline(&line_buf, &line_buf_size, cpu_stats_file);

	/* Loop through until we are done with the file. */
	while (line_size >= 0)
	{
		if (strstr(line_buf, "cpu") != NULL)
		{
			sscanf(line_buf, scan_fmt, cpu_name,
						&usermode_normal_process,
						&usermode_niced_process,
						&kernelmode_process,
						&idle_mode,
						&io_completion);
			total_cpu_time = usermode_normal_process + usermode_niced_process + kernelmode_process + idle_mode + io_completion;
			break;
		}

		/* Free the allocated line buffer */
		if (line_buf != NULL)
		{
			free(line_buf);
			line_buf = NULL;
		}

		/* Get the next line */
		line_size = getline(&line_buf, &line_buf_size, cpu_stats_file);
	}

	/* Free the allocated line buffer */
	if (line_buf != NULL)
	{
		free(line_buf);
		line_buf = NULL;
	}

	fclose(cpu_stats_file);

	return total_cpu_time;
}

/* Read CPU and memory informations all processes and store in
 * linked list for further processing */
void ReadCPUMemoryUsage(int sample)
{
	FILE *fpstat;
	struct dirent *ent, dbuf;
	char  file_name[MAXPGPATH];
	long utime_ticks, stime_ticks;
	char process_name[MAXPGPATH] = {0};
	int pid = 0;
	long unsigned int mem_rss = 0;
	unsigned  long long  process_up_since = 0;
	int        HZ = 100;
	long       tlk = -1;
	struct     sysinfo s_info;
	long       sys_uptime = 0;
	DIR        *dirp = NULL;

	/* First get the HZ value from system as it may vary from system to system */
	tlk = sysconf(_SC_CLK_TCK);

	if (tlk != -1 && tlk > 0)
	    HZ = (int)tlk;

	if (sysinfo(&s_info) == 0)
		sys_uptime = s_info.uptime;

	dirp = opendir(PROC_FILE_SYSTEM_PATH);

	if (!dirp)
	{
		ereport(DEBUG1, (errmsg("Error opening /proc directory")));
		return;
	}

	while (readdir_r(dirp, &dbuf, &ent) == 0)
	{
		memset(file_name, 0x00, MAXPGPATH);

		if (!ent)
			break;

		if (!isdigit(*ent->d_name))
			continue;

		sprintf(file_name,"/proc/%s/stat", ent->d_name);

		fpstat = fopen(file_name, "r");
		if (fpstat == NULL)
			continue;

		if (fscanf(fpstat, "%d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu"
					"%lu %*d %*d %*d %*d %*d %*d %llu %*u %ld",
					&pid, process_name, &utime_ticks, &stime_ticks, &process_up_since, &mem_rss) == EOF)
		{
			ereport(DEBUG1,
				(errmsg("Error in parsing file '/proc/%d/stat'", pid)));
			fclose(fpstat);
			continue;
		}

		if (sample == READ_PROCESS_CPU_USAGE_FIRST_SAMPLE)
		{
			iter = (node_t *) malloc(sizeof(node_t));
			if (iter == NULL)
			{
				fclose(fpstat);
				continue;
			}

			iter->pid = pid;
			memcpy(iter->name, process_name, MAXPGPATH);
			iter->process_cpu_sample_1 = utime_ticks + stime_ticks;
			iter->rss_memory = mem_rss;
			process_up_since = (unsigned long long)((unsigned long long)sys_uptime - (process_up_since/HZ));
			iter->process_up_since_seconds = process_up_since;
			iter->next = NULL;
			if (head == NULL)
				head = iter;
			else
				prev->next = iter;
			prev = iter;
		}
		else
		{
			node_t * current = head;
			while (current != NULL)
			{
				if (current->pid == atoi(ent->d_name))
				{
					current->process_cpu_sample_2 = utime_ticks + stime_ticks;
					break;
				}
				else
					current = current->next;
			}
		}

		fclose(fpstat);
		fpstat = NULL;
	}

	closedir(dirp);
}

void ReadCPUMemoryByProcess(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum      values[Natts_cpu_memory_info_by_process];
	bool       nulls[Natts_cpu_memory_info_by_process];
	char       command[MAXPGPATH];
	int        process_pid = 0;
	int        no_processor = 0;
	float4     cpu_usage = 0.0;
	float4     memory_usage = 0.0;
	long page_size_bytes = 0;
	long long unsigned int     total_memory;
	long long unsigned int     rss_memory;
	long long unsigned int     running_since;
	node_t *del_iter = NULL;
	node_t *current  = NULL;

	memset(nulls, 0, sizeof(nulls));
	memset(command, 0, MAXPGPATH);

	no_processor =  ReadTotalProcessors();
	total_memory = ReadTotalPhysicalMemory();
	total_cpu_usage_1 = ReadTotalCPUUsage();
	/* Read the first sample for cpu and memory usage by each process */
	ReadCPUMemoryUsage(READ_PROCESS_CPU_USAGE_FIRST_SAMPLE);
	usleep(100000);
	/* Read the second sample for cpu and memory usage by each process */
	total_cpu_usage_2 = ReadTotalCPUUsage();
	ReadCPUMemoryUsage(READ_PROCESS_CPU_USAGE_SECOND_SAMPLE);

	page_size_bytes = sysconf(_SC_PAGESIZE);

	// Iterate through head and read all the informations */
	current = head;

	// Process the CPU and memory information from linked list and free it once processed */
	while (current != NULL)
	{
		process_pid = current->pid;
		memcpy(command, current->name, MAXPGPATH);
		cpu_usage = (no_processor) * (current->process_cpu_sample_2 - current->process_cpu_sample_1) * 100 / (float) (total_cpu_usage_2 - total_cpu_usage_1);
		rss_memory = current->rss_memory * page_size_bytes;
		memory_usage = (rss_memory/(float)total_memory)*100;
		running_since = current->process_up_since_seconds;
		memory_usage = fl_round(memory_usage);
		cpu_usage = fl_round(cpu_usage);

		values[Anum_process_pid] = Int32GetDatum(process_pid);
		values[Anum_process_name] = CStringGetTextDatum(command);
		values[Anum_percent_cpu_usage] = Float4GetDatum(cpu_usage);
		values[Anum_percent_memory_usage] = Float4GetDatum(memory_usage);
		values[Anum_process_memory_bytes] = UInt64GetDatum((uint64)rss_memory);
		values[Anum_process_running_since] = UInt64GetDatum((uint64)(running_since));

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);

		//reset the value again
		memset(command, 0, MAXPGPATH);
		process_pid = 0;
		cpu_usage = 0.0;
		memory_usage = 0.0;
		running_since = 0;
		rss_memory = 0;

		del_iter = current;
		current = current->next;
		if (del_iter != NULL)
		{
			free(del_iter);
			del_iter = NULL;
		}
	}

	head = NULL;
	prev = NULL;
	iter = NULL;
}
