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

void ReadCPUMemoryByProcess(Tuplestorestate *tupstore, TupleDesc tupdesc, text *process_val)
{
	char       *output;
	char       *found;
	char       *new_line_token;
	Datum      values[Natts_cpu_memory_info_by_process];
	bool       nulls[Natts_cpu_memory_info_by_process];
	char       str_val[MAXPGPATH];
	char       cmd_name[MAX_BUFFER_SIZE];
	char       command[MAXPGPATH];
	char       user_name[MAXPGPATH];
	int        process_pid = 0;
	int        input_str_len;
	float4     cpu_usage = 0;
	float4     memory_usage = 0;
	const char *scan_fmt = "%d %s %f %f %[^\n]";
	char      *input_str = (char *)VARDATA_ANY(process_val);

	if (input_str == NULL)
	{
		ereport(DEBUG1, (errmsg("NULL value received in input for process identifier")));
		return;
	}

	input_str_len = (int)VARSIZE_ANY_EXHDR(process_val);
	if (input_str_len > 0)
	{
		memset(str_val, 0x00, MAXPGPATH);
		memcpy(str_val, input_str, input_str_len);
	}

	memset(nulls, 0, sizeof(nulls));
	memset(cmd_name, 0, MAX_BUFFER_SIZE);
	memset(command, 0, MAXPGPATH);
	memset(user_name, 0, MAXPGPATH);

	// form the command based on input argument given by user
	if (stringIsNumber(str_val))
		sprintf(command, "ps -p %s -o 'pid,user,%%cpu,%%mem,cmd'", str_val);
	else
		sprintf(command, "ps -C %s -o 'pid,user,%%cpu,%%mem,cmd'", str_val);

	/* execute the command and get the output */
	output = runCommand(command);

	if (output != NULL)
	{
		/* Parse the command output and find operating system information */
		for (new_line_token = strtok(output,"\n"); new_line_token != NULL; new_line_token = strtok(NULL, "\n"))
		{
			/* No need to process the line if it is empty */
			if (IS_EMPTY_STR(new_line_token))
				continue;

			found = strstr(new_line_token, "\%CPU");

			// skip the header of the output
			if (found != NULL)
				continue;

			sscanf(new_line_token, scan_fmt, &process_pid, user_name, &cpu_usage, &memory_usage, cmd_name);

			values[Anum_process_pid] = Int32GetDatum(process_pid);
			values[Anum_process_user_name] = CStringGetTextDatum(user_name);
			values[Anum_process_cpu_usage] = Float4GetDatum(cpu_usage);
			values[Anum_process_memory_usage] = Float4GetDatum(memory_usage);
			values[Anum_process_command] = CStringGetTextDatum(cmd_name);

			tuplestore_putvalues(tupstore, tupdesc, values, nulls);

			//reset the value again
			memset(cmd_name, 0, MAX_BUFFER_SIZE);
			memset(user_name, 0, MAXPGPATH);
			process_pid = 0;
			cpu_usage = 0;
			memory_usage = 0;
		}
	}

	/* Free the allocated buffer) */
	if (output != NULL)
	{
		pfree(output);
		output = NULL;
	}
}
