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
	unsigned long long     vsize;
	long long unsigned int swap_bytes;
	long long unsigned int io_read_bytes;
	long long unsigned int io_write_bytes;
	bool                   has_swap;
	bool                   has_io;
	struct node * next;
} node_t;

static node_t *head = NULL;
static node_t *prev = NULL;
static node_t *iter = NULL;

/* Function used to get number of processor count */
int ReadTotalProcessors(void);
/* Function used to get total physical RAM available on system */
uint64 ReadTotalPhysicalMemory(void);
/* Function used to read total cpu usage for each process */
uint64 ReadTotalCPUUsage(void);
/* Function used to read total memory usage for each process */
void ReadCPUMemoryUsage(int sample);
/* Function used to read swap usage from /proc/<pid>/status */
static bool ReadProcessSwap(int pid, long long unsigned int *swap_bytes);
/* Function used to read IO stats from /proc/<pid>/io */
static bool ReadProcessIO(int pid,
		long long unsigned int *read_bytes,
		long long unsigned int *write_bytes);

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

		/* Get the next line */
		line_size = getline(&line_buf, &line_buf_size, memory_file);
	}

	free(line_buf);

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
	char       cpu_name[MAXPGPATH + 1];
	uint64     total_cpu_time = 0;
	uint64     usermode_normal_process = 0;
	uint64     usermode_niced_process = 0;
	uint64     kernelmode_process = 0;
	uint64     idle_mode = 0;
	uint64     io_completion = 0;
	const char *scan_fmt = "%" CppAsString2(MAXPGPATH) "s %llu %llu %llu %llu %llu";

	memset(cpu_name, 0, MAXPGPATH + 1);

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

		/* Get the next line */
		line_size = getline(&line_buf, &line_buf_size, cpu_stats_file);
	}

	free(line_buf);

	fclose(cpu_stats_file);

	return total_cpu_time;
}

/* Read CPU and memory informations all processes and store in
 * linked list for further processing */
void ReadCPUMemoryUsage(int sample)
{
	FILE *fpstat;
	struct dirent *ent;
	char  file_name[MAXPGPATH];
	unsigned long utime_ticks, stime_ticks;
	char process_name[MAXPGPATH + 1] = {0};
	char stat_line[4096];
	int pid = 0;
	long unsigned int mem_rss = 0;
	unsigned long long vsize = 0;
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

	while ((ent = readdir(dirp)) != NULL)
	{
		memset(file_name, 0x00, MAXPGPATH);

		if (!isdigit(*ent->d_name))
			continue;

		snprintf(file_name, MAXPGPATH, "/proc/%s/stat", ent->d_name);

		fpstat = fopen(file_name, "r");
		if (fpstat == NULL)
			continue;

		/*
		 * Parse /proc/<pid>/stat robustly. The comm field (field 2)
		 * is wrapped in parentheses and may contain spaces or even
		 * ')' chars.  The kernel guarantees the first '(' and last
		 * ')' in the line delimit comm, so we locate those markers
		 * and sscanf the numeric fields after ')'.
		 */
		if (fgets(stat_line, sizeof(stat_line), fpstat) == NULL)
		{
			fclose(fpstat);
			continue;
		}

		{
			char *open_paren;
			char *close_paren;
			char *after_comm;
			size_t name_len;

			open_paren = strchr(stat_line, '(');
			close_paren = strrchr(stat_line, ')');
			if (open_paren == NULL || close_paren == NULL ||
				close_paren <= open_paren)
			{
				fclose(fpstat);
				continue;
			}

			/* Extract pid from before '(' */
			if (sscanf(stat_line, "%d", &pid) != 1)
			{
				fclose(fpstat);
				continue;
			}

			/* Extract comm from between '(' and last ')' */
			open_paren++;  /* skip '(' */
			name_len = close_paren - open_paren;
			if (name_len >= MAXPGPATH)
				name_len = MAXPGPATH - 1;
			memcpy(process_name, open_paren, name_len);
			process_name[name_len] = '\0';

			/* Parse numeric fields after ") " */
			after_comm = close_paren + 1;
			if (sscanf(after_comm,
					   " %*c %*d %*d %*d %*d %*d %*u"
					   " %*u %*u %*u %*u"
					   " %lu %lu"
					   " %*d %*d %*d %*d %*d %*d"
					   " %llu %llu %lu",
					   &utime_ticks, &stime_ticks,
					   &process_up_since, &vsize,
					   &mem_rss) != 5)
			{
				ereport(DEBUG1,
					(errmsg("Error parsing fields in"
							" '/proc/%d/stat'", pid)));
				fclose(fpstat);
				continue;
			}
		}

		if (sample == READ_PROCESS_CPU_USAGE_FIRST_SAMPLE)
		{
			iter = (node_t *) malloc(sizeof(node_t));
			if (iter == NULL)
			{
				fclose(fpstat);
				continue;
			}
			/* Zero-initialize so process_cpu_sample_2 is 0
			 * for processes that disappear between samples */
			memset(iter, 0, sizeof(node_t));

			iter->pid = pid;
			strncpy(iter->name, process_name, MAXPGPATH);
			iter->name[MAXPGPATH - 1] = '\0';
			iter->process_cpu_sample_1 = utime_ticks + stime_ticks;
			iter->rss_memory = mem_rss;
			iter->vsize = vsize;
			process_up_since = (unsigned long long)((unsigned long long)sys_uptime - (process_up_since/HZ));
			iter->process_up_since_seconds = process_up_since;
			iter->has_swap = ReadProcessSwap(pid, &iter->swap_bytes);
			iter->has_io = ReadProcessIO(pid,
					&iter->io_read_bytes,
					&iter->io_write_bytes);
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

/* Read swap usage from /proc/<pid>/status */
static bool ReadProcessSwap(int pid, long long unsigned int *swap_bytes)
{
	FILE       *fp;
	char       file_name[MAXPGPATH];
	char       *line_buf = NULL;
	size_t     line_buf_size = 0;
	ssize_t    line_size;
	bool       found = false;

	snprintf(file_name, MAXPGPATH, "/proc/%d/status", pid);
	fp = fopen(file_name, "r");
	if (!fp)
		return false;

	line_size = getline(&line_buf, &line_buf_size, fp);
	while (line_size >= 0)
	{
		if (strstr(line_buf, "VmSwap:") != NULL)
		{
			long long unsigned int val = 0;
			if (sscanf(line_buf, "VmSwap: %llu", &val) == 1)
			{
				*swap_bytes = val * 1024; /* convert kB to bytes */
				found = true;
			}
			break;
		}

		line_size = getline(&line_buf, &line_buf_size, fp);
	}

	free(line_buf);
	fclose(fp);
	return found;
}

/* Read IO stats from /proc/<pid>/io */
static bool ReadProcessIO(int pid,
		long long unsigned int *read_bytes,
		long long unsigned int *write_bytes)
{
	FILE       *fp;
	char       file_name[MAXPGPATH];
	char       *line_buf = NULL;
	size_t     line_buf_size = 0;
	ssize_t    line_size;
	bool       found_read = false;
	bool       found_write = false;

	snprintf(file_name, MAXPGPATH, "/proc/%d/io", pid);
	fp = fopen(file_name, "r");
	if (!fp)
		return false;

	line_size = getline(&line_buf, &line_buf_size, fp);
	while (line_size >= 0)
	{
		if (strstr(line_buf, "read_bytes:") != NULL &&
			strstr(line_buf, "cancelled") == NULL)
		{
			if (sscanf(line_buf, "read_bytes: %llu", read_bytes) == 1)
				found_read = true;
		}
		else if (strstr(line_buf, "write_bytes:") != NULL &&
			strstr(line_buf, "cancelled") == NULL)
		{
			if (sscanf(line_buf, "write_bytes: %llu", write_bytes) == 1)
				found_write = true;
		}

		if (found_read && found_write)
			break;

		line_size = getline(&line_buf, &line_buf_size, fp);
	}

	free(line_buf);
	fclose(fp);
	return (found_read && found_write);
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
	pg_usleep(100000);
	CHECK_FOR_INTERRUPTS();
	/* Read the second sample for cpu and memory usage by each process */
	total_cpu_usage_2 = ReadTotalCPUUsage();
	ReadCPUMemoryUsage(READ_PROCESS_CPU_USAGE_SECOND_SAMPLE);

	page_size_bytes = sysconf(_SC_PAGESIZE);
	if (page_size_bytes <= 0)
		page_size_bytes = 4096;  /* fallback to common default */

	// Iterate through head and read all the informations */
	current = head;

	// Process the CPU and memory information from linked list and free it once processed */
	while (current != NULL)
	{
		process_pid = current->pid;
		memcpy(command, current->name, MAXPGPATH);

		/* Skip if second sample < first (process died/PID reused) */
		if (current->process_cpu_sample_2 <
			current->process_cpu_sample_1)
			cpu_usage = 0.0;
		/* Guard against div-by-zero or underflow in total CPU */
		else if (total_cpu_usage_2 <= total_cpu_usage_1)
			cpu_usage = 0.0;
		else
			cpu_usage = (float)no_processor *
				(float)(current->process_cpu_sample_2 -
				 current->process_cpu_sample_1) *
				100.0f / (float)(total_cpu_usage_2 -
				 total_cpu_usage_1);

		rss_memory = current->rss_memory * page_size_bytes;
		/* Guard against division by zero when total memory is unavailable */
		if (total_memory == 0)
			memory_usage = 0.0;
		else
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

		/* virtual memory bytes */
		values[Anum_process_virtual_memory_bytes] =
			UInt64GetDatum((uint64)(current->vsize));

		/* swap usage */
		if (current->has_swap)
			values[Anum_process_swap_usage_bytes] =
				UInt64GetDatum((uint64)(current->swap_bytes));
		else
			nulls[Anum_process_swap_usage_bytes] = true;

		/* IO read/write bytes */
		if (current->has_io)
		{
			values[Anum_process_io_read_bytes] =
				UInt64GetDatum((uint64)(current->io_read_bytes));
			values[Anum_process_io_write_bytes] =
				UInt64GetDatum((uint64)(current->io_write_bytes));
		}
		else
		{
			nulls[Anum_process_io_read_bytes] = true;
			nulls[Anum_process_io_write_bytes] = true;
		}

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);

		//reset the value again
		memset(command, 0, MAXPGPATH);
		process_pid = 0;
		cpu_usage = 0.0;
		memory_usage = 0.0;
		running_since = 0;
		rss_memory = 0;
		nulls[Anum_process_swap_usage_bytes] = false;
		nulls[Anum_process_io_read_bytes] = false;
		nulls[Anum_process_io_write_bytes] = false;

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
