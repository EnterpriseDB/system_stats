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

void ReadProcessInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	char       *output;
	char       *new_token;
	char       process_value[MAXPGPATH];
	Datum      values[Natts_cpu_usage_stats];
	bool       nulls[Natts_cpu_usage_stats];
	int        active_processes = 0;
	int        running_processes = 0;
	int        sleeping_processes = 0;
	int        stopped_processes = 0;
	int        zombie_processes = 0;

	memset(nulls, 0, sizeof(nulls));
	memset(process_value, 0, MAXPGPATH);

	/* Run the command and read the output */
	output = runCommand(PROCESS_INFO_COMMAND);

	if (output)
	{
		/* Parse the command output and read process informations */
		char *found = strstr(output,":");
		found = (found + 1);
		for (new_token = strtok(found,","); new_token != NULL; new_token = strtok(NULL, ","))
		{
			char *process_type = strstr(trimStr(new_token), " ");
			process_type = trimStr((process_type+1));

			if (strstr(process_type, "total") != NULL)
			{
				memcpy(process_value, new_token, (process_type-new_token));
				active_processes = (int)atoi(trimStr(process_value));
			}
			if (strstr(process_type, "running") != NULL)
			{
				memcpy(process_value, new_token, (process_type-new_token));
				running_processes = (int)atoi(trimStr(process_value));
			}
			if (strstr(process_type, "sleeping") != NULL)
			{
				memcpy(process_value, new_token, (process_type-new_token));
				sleeping_processes = (int)atoi(trimStr(process_value));
			}
			if (strstr(process_type, "stopped") != NULL)
			{
				memcpy(process_value, new_token, (process_type-new_token));
				stopped_processes = (int)atoi(trimStr(process_value));
			}
			if (strstr(process_type, "zombie") != NULL)
			{
				memcpy(process_value, new_token, (process_type-new_token));
				zombie_processes = (int)atoi(trimStr(process_value));
			}

			// reset the value again
			memset(process_value, 0, MAXPGPATH);
		}

		values[Anum_active_processes] = Int32GetDatum(active_processes);
		values[Anum_running_processes] = Int32GetDatum(running_processes);
		values[Anum_sleeping_processes] = Int32GetDatum(sleeping_processes);
		values[Anum_stopped_processes] = Int32GetDatum(stopped_processes);
		values[Anum_zombie_processes] = Int32GetDatum(zombie_processes);

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);
	}
	else
		return;

	/* Free the allocated buffer) */
	if (output != NULL)
	{
		pfree(output);
		output = NULL;
	}
}
