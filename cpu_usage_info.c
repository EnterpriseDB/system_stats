/*------------------------------------------------------------------------
 * cpu_usage_info.c
 *              System CPU usage information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"

void ReadCPUUsageStatistics(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	char       *output;
	FILE       *cpu_stats_file;
	char       *line_buf = NULL;
	size_t     line_buf_size = 0;
	ssize_t    line_size;
	Datum      values[Natts_cpu_usage_stats];
	bool       nulls[Natts_cpu_usage_stats];
	char       cpu_name[MAXPGPATH];
	uint64     usermode_normal_process = 0;
	uint64     usermode_niced_process = 0;
	uint64     kernelmode_process = 0;
	uint64     idle_mode = 0;
	uint64     io_completion = 0;
	uint64     servicing_irq = 0;
	uint64     servicing_softirq = 0;
	const char *scan_fmt = "%s %llu %llu %llu %llu %llu %llu %llu";
	int        HZ = 100;

	memset(nulls, 0, sizeof(nulls));
	memset(cpu_name, 0, MAXPGPATH);

	/* First get the HZ value from system as it may vary from system to system */
	output = runCommand(GET_HZ_CONFIGURED_COMMAND);

	if (output)
		HZ = atoi(output);

	/* Free the allocated buffer) */
	if (output != NULL)
	{
		pfree(output);
		output = NULL;
	}

	cpu_stats_file = fopen(CPU_USAGE_STATS_FILENAME, "r");

	if (!cpu_stats_file)
	{
		char cpu_stats_file_name[MAXPGPATH];
		snprintf(cpu_stats_file_name, MAXPGPATH, "%s", CPU_USAGE_STATS_FILENAME);

		ereport(WARNING,
				(errcode_for_file_access(),
				errmsg("can not open file %s for reading cpu usage statistics",
					cpu_stats_file_name)));
		return;
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
						&io_completion,
						&servicing_irq,
						&servicing_softirq);

			// Convert value to miliseconds as all values are in jiffie
			usermode_normal_process = (usermode_normal_process * 1000)/HZ;
			usermode_niced_process = (usermode_niced_process * 1000)/HZ;
			kernelmode_process = (kernelmode_process * 1000)/HZ;
			idle_mode = (idle_mode * 1000)/HZ;
			io_completion = (io_completion * 1000)/HZ;
			servicing_irq = (servicing_irq * 1000)/HZ;
			servicing_softirq = (servicing_softirq * 1000)/HZ;

			values[Anum_cpu_name] = CStringGetTextDatum(cpu_name);
			values[Anum_usermode_normal_process] = Int64GetDatumFast(usermode_normal_process);
			values[Anum_usermode_niced_process] = Int64GetDatumFast(usermode_niced_process);
			values[Anum_kernelmode_process] = Int64GetDatumFast(kernelmode_process);
			values[Anum_idle_mode] = Int64GetDatumFast(idle_mode);
			values[Anum_io_completion] = Int64GetDatumFast(io_completion);
			values[Anum_servicing_irq] = Int64GetDatumFast(servicing_irq);
			values[Anum_servicing_softirq] = Int64GetDatumFast(servicing_softirq);

			tuplestore_putvalues(tupstore, tupdesc, values, nulls);

			//reset the value again
			memset(cpu_name, 0, MAXPGPATH);
			usermode_normal_process = 0;
			usermode_niced_process = 0;
			kernelmode_process = 0;
			idle_mode = 0;
			io_completion = 0;
			servicing_irq = 0;
			servicing_softirq = 0;
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
}
