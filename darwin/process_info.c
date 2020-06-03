/*------------------------------------------------------------------------
 * process_info.c
 *              System process information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"

#include <sys/types.h>
#include <sys/sysctl.h>

extern int get_process_list(struct kinfo_proc **proc_list, size_t *proc_count);

void ReadProcessInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	size_t        num_processes = 0;
	int           total_processes = 0;
	int           running_processes = 0;
	int           sleeping_processes = 0;
	int           stopped_processes = 0;
	int           zombie_processes = 0;
	struct        kinfo_proc *proc_list = NULL;
	struct        kinfo_proc *proc_list_addr = NULL;
	size_t        index;
	Datum         values[Natts_process_info];
	bool          nulls[Natts_process_info];

	memset(nulls, 0, sizeof(nulls));

	if (get_process_list(&proc_list, &num_processes) != 0)
	{
		ereport(DEBUG1, (errmsg("Error while getting process information list from proc")));
		return;
	}

	total_processes = (int)num_processes;

	/* Save the allocated buffer pointer so that it can be freed at later */
	proc_list_addr = proc_list;

	/* Iterate total processes to find its status */
	for (index = 0; index < total_processes; index++)
	{
		if (proc_list->kp_proc.p_pid > 0 && (int)proc_list->kp_proc.p_stat == SRUN)
			running_processes++;
		if (proc_list->kp_proc.p_pid > 0 && (int)proc_list->kp_proc.p_stat == SSLEEP)
			sleeping_processes++;
		if (proc_list->kp_proc.p_pid > 0 && (int)proc_list->kp_proc.p_stat == SSTOP)
			stopped_processes++;
		if (proc_list->kp_proc.p_pid > 0 && (int)proc_list->kp_proc.p_stat == SZOMB)
			zombie_processes++;

		proc_list++;
	}

	free(proc_list_addr);

	values[Anum_no_of_total_processes]      = Int32GetDatum(total_processes);
	values[Anum_no_of_running_processes]    = Int32GetDatum(running_processes);
	values[Anum_no_of_sleeping_processes]   = Int32GetDatum(sleeping_processes);
	values[Anum_no_of_stopped_processes]    = Int32GetDatum(stopped_processes);
	values[Anum_no_of_zombie_processes]     = Int32GetDatum(zombie_processes);

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);
}
