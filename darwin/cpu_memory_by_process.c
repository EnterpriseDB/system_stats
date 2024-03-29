/*------------------------------------------------------------------------
 * cpu_memory_by_process.c
 *              CPU and memory usage of all processes
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"

#include <sys/types.h>
#include <sys/sysctl.h>
#include <unistd.h>

#include <mach/mach.h>
#include <mach/vm_page_size.h>

#include <libproc.h>
#include <sys/proc_info.h>

extern int get_process_list(struct kinfo_proc **proc_list, size_t *proc_count);
extern uint64 find_cpu_times(void);
void CreateCPUMemoryList(int sample);
void findProcess(void);

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
	char name[MAXPGPATH];
	int  process_owned_by_user;
	struct node * next;
} node_t;

node_t *head = NULL;
node_t *prev = NULL;
node_t *iter = NULL;

/* Function used to create the data structure for each process CPU and memory usage information */
void CreateCPUMemoryList(int sample)
{
	struct     kinfo_proc *proclist = NULL;
	struct     kinfo_proc *org_proc_addr = NULL;
	size_t     num_processes;
	size_t     index;

	if (get_process_list(&proclist, &num_processes) != 0)
		return;

	// save the address of proclist so we can free it later
	org_proc_addr = proclist;
	for (index = 0; index < num_processes; index++)
	{
		pid_t pid;
		struct proc_taskinfo pti;
		int ret_val = 0;
 
		pid = (pid_t)proclist->kp_proc.p_pid;
		ret_val = proc_pidinfo(proclist->kp_proc.p_pid, PROC_PIDTASKINFO, 0, &pti, sizeof(pti));
		if ((ret_val <= 0) || ((unsigned long)ret_val < sizeof(pti)))
			elog(DEBUG1, "proc_pidinfo[pid: %d] return value: [%d]", (int)pid, ret_val);

		if (sample == READ_PROCESS_CPU_USAGE_FIRST_SAMPLE)
		{
			iter = (node_t *) malloc(sizeof(node_t));
			if (iter == NULL)
			{
				proclist++;
				continue;
			}

			memset(iter, 0x00, sizeof(node_t));
			iter->pid = proclist->kp_proc.p_pid;
			memcpy(iter->name, proclist->kp_proc.p_comm, MAXPGPATH);
			if ((ret_val <= 0) || ((unsigned long)ret_val < sizeof(pti)))
				iter->process_owned_by_user = 0;
			else
			{
				iter->process_cpu_sample_1 = pti.pti_total_user + pti.pti_total_system;
				iter->rss_memory = pti.pti_resident_size;
				iter->process_owned_by_user = 1;
			}

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
				if (current->pid == (int)pid)
				{
					if (!(ret_val <= 0) || ((unsigned long)ret_val < sizeof(pti)))
						current->process_cpu_sample_2 = pti.pti_total_user + pti.pti_total_system;
					break;
				}
				else
					current = current->next;
			}
		}

		proclist++;
	}

	free(org_proc_addr);
}

void ReadCPUMemoryByProcess(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum      values[Natts_cpu_memory_info_by_process];
	bool       nulls[Natts_cpu_memory_info_by_process];
	char       command[MAXPGPATH];
	int        process_pid = 0;
	float4     cpu_usage = 0.0;
	float4     memory_usage = 0.0;
	int        desc[2];
	uint64     total_memory;
	uint64     page_size_bytes;
	int        num_cpus = 0;
	size_t     size_cpus = sizeof(num_cpus);
	size_t     p_size = sizeof(page_size_bytes);
	long long unsigned int     rss_memory;
	node_t     *del_iter = NULL;
	node_t     *current  = NULL;

	memset(nulls, 0, sizeof(nulls));
	memset(command, 0, MAXPGPATH);

	desc[0] = CTL_HW;
	desc[1] = HW_MEMSIZE;

	/* Find the total physical memory available with the system */
	if (sysctl(desc, 2, &total_memory, &p_size, NULL, 0))
		ereport(DEBUG1, (errmsg("Error while getting total memory information")));

	/* Get the total number of CPUs */
	if (sysctlbyname("hw.ncpu", &num_cpus, &size_cpus, 0, 0) == -1)
		ereport(DEBUG1, (errmsg("Error while getting total CPU cores")));

	/* Get the page size */
	if (sysctlbyname("hw.pagesize", &page_size_bytes, &p_size, 0, 0) == -1)
		ereport(DEBUG1, (errmsg("Error while getting page size information")));

	total_cpu_usage_1 = find_cpu_times();
	/* Read the first sample for cpu and memory usage by each process */
	CreateCPUMemoryList(READ_PROCESS_CPU_USAGE_FIRST_SAMPLE);
	usleep(100000);
	/* Read the second sample for cpu and memory usage by each process */
	total_cpu_usage_2 = find_cpu_times();
	CreateCPUMemoryList(READ_PROCESS_CPU_USAGE_SECOND_SAMPLE);

	// Iterate through head and read all the informations */
	current = head;

	// Process the CPU and memory information from linked list and free it once processed */
	while (current != NULL)
	{
		process_pid = current->pid;
		memcpy(command, current->name, MAXPGPATH);
		if (current->process_owned_by_user)
		{
			float diff_sample = (float)(current->process_cpu_sample_2 - current->process_cpu_sample_1) / 1000000000.0;
			cpu_usage = (num_cpus) * (diff_sample) * 100 / (float) ((total_cpu_usage_2 - total_cpu_usage_1)/CLK_TCK);
			cpu_usage = (float)((int)(cpu_usage * 100 + 0.5))/100;
			rss_memory = current->rss_memory;
			memory_usage = (rss_memory/(float)total_memory)*100;
			memory_usage = (float)((int)(memory_usage * 100 + 0.5))/100;
			values[Anum_percent_cpu_usage] = Float4GetDatum(cpu_usage);
			values[Anum_percent_memory_usage] = Float4GetDatum(memory_usage);
			values[Anum_process_memory_bytes] = UInt64GetDatum((uint64)rss_memory);
		}
		else
		{
			nulls[Anum_percent_cpu_usage] = true;
			nulls[Anum_percent_memory_usage] = true;
			nulls[Anum_process_memory_bytes] = true;
		}

		values[Anum_process_pid] = Int32GetDatum(process_pid);
		values[Anum_process_name] = CStringGetTextDatum(command);

		nulls[Anum_process_running_since] = true;

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);

		//reset the value again
		memset(command, 0, MAXPGPATH);
		process_pid = 0;
		cpu_usage = 0.0;
		memory_usage = 0.0;

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
