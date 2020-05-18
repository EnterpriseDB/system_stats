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

void ReadProcessInformations(Tuplestorestate *tupstore, TupleDesc tupdesc);

void ReadProcessInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	int           active_processes = 0;
	int           running_processes = 0;
	int           sleeping_processes = 0;
	int           stopped_processes = 0;
	int           zombie_processes = 0;
	int           total_threads = 0;
	Datum         values[Natts_process_info];
	bool          nulls[Natts_process_info];

	memset(nulls, 0, sizeof(nulls));

	if (read_process_status(&active_processes, &running_processes, &sleeping_processes,
							&stopped_processes, &zombie_processes, &total_threads))
	{
		values[Anum_no_of_total_processes] = Int32GetDatum(active_processes);
		values[Anum_no_of_running_processes] = Int32GetDatum(running_processes);
		values[Anum_no_of_sleeping_processes] = Int32GetDatum(sleeping_processes);
		values[Anum_no_of_stopped_processes] = Int32GetDatum(stopped_processes);
		values[Anum_no_of_zombie_processes] = Int32GetDatum(zombie_processes);
	}
	else
	{
		nulls[Anum_no_of_total_processes] = true;
		nulls[Anum_no_of_running_processes] = true;
		nulls[Anum_no_of_sleeping_processes] = true;
		nulls[Anum_no_of_stopped_processes] = true;
		nulls[Anum_no_of_zombie_processes] = true;
	}

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);
}
