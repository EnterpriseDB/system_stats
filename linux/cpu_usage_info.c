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

#include <unistd.h>

void ReadCPUUsageStatistics(Tuplestorestate *tupstore, TupleDesc tupdesc);

struct cpu_stat
{
	long long int usermode_normal_process;
	long long int usermode_niced_process;
	long long int kernelmode_process;
	long long int idle_mode;
	long long int io_completion;
	long long int servicing_irq;
	long long int servicing_softirq;
};

void cpu_stat_information(struct cpu_stat* cpu_stat);
/* Function used to get CPU state information for each mode of operation */
void cpu_stat_information(struct cpu_stat* cpu_stat)
{
	FILE       *cpu_stats_file;
	char       *line_buf = NULL;
	size_t     line_buf_size = 0;
	ssize_t    line_size;
	long long int     usermode_normal_process = 0;
	long long int     usermode_niced_process = 0;
	long long int     kernelmode_process = 0;
	long long int     idle_mode = 0;
	long long int     io_completion = 0;
	long long int     servicing_irq = 0;
	long long int     servicing_softirq = 0;
	const char *scan_fmt = "%*s %llu %llu %llu %llu %llu %llu %llu";

	cpu_stats_file = fopen(CPU_USAGE_STATS_FILENAME, "r");

	if (!cpu_stats_file)
	{
		char cpu_stats_file_name[MAXPGPATH];
		snprintf(cpu_stats_file_name, MAXPGPATH, "%s", CPU_USAGE_STATS_FILENAME);

		ereport(WARNING,
				(errcode_for_file_access(),
				errmsg("can not open file %s for reading cpu usage statistics",
					cpu_stats_file_name)));

		cpu_stat->usermode_normal_process = 0;
		cpu_stat->usermode_niced_process = 0;
		cpu_stat->kernelmode_process = 0;
		cpu_stat->idle_mode = 0;
		cpu_stat->io_completion = 0;
		cpu_stat->servicing_irq = 0;
		cpu_stat->servicing_softirq = 0;
		return;
	}

	/* Get the first line of the file. */
	line_size = getline(&line_buf, &line_buf_size, cpu_stats_file);

	/* Loop through until we are done with the file. */
	while (line_size >= 0)
	{
		if (strstr(line_buf, "cpu") != NULL)
		{
			sscanf(line_buf, scan_fmt, &usermode_normal_process,
						&usermode_niced_process,
						&kernelmode_process,
						&idle_mode,
						&io_completion,
						&servicing_irq,
						&servicing_softirq);

			cpu_stat->usermode_normal_process = usermode_normal_process;
			cpu_stat->usermode_niced_process = usermode_niced_process;
			cpu_stat->kernelmode_process = kernelmode_process;
			cpu_stat->idle_mode = idle_mode;
			cpu_stat->io_completion = io_completion;
			cpu_stat->servicing_irq = servicing_irq;
			cpu_stat->servicing_softirq = servicing_softirq;
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
}

void ReadCPUUsageStatistics(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum             values[Natts_cpu_usage_stats];
	bool              nulls[Natts_cpu_usage_stats];
	struct            cpu_stat first_sample, second_sample;
	long long int     delta_usermode_normal_process = 0;
	long long int     delta_usermode_niced_process = 0;
	long long int     delta_kernelmode_process = 0;
	long long int     delta_idle_mode = 0;
	long long int     delta_io_completion = 0;
	long long int     delta_servicing_irq = 0;
	long long int     delta_servicing_softirq = 0;
	long long int     total_delta = 0;
	float             scale = 100.0;
	float             f_usermode_normal_process = 0.00;
	float             f_usermode_niced_process = 0.00;
	float             f_kernelmode_process = 0.00;
	float             f_idle_mode = 0.00;
	float             f_io_completion = 0.00;
	float             f_servicing_irq = 0.00;
	float             f_servicing_softirq = 0.00;

	memset(nulls, 0, sizeof(nulls));

	/* Take the first sample regarding cpu usage statistics */
	cpu_stat_information(&first_sample);
	/* sleep for the 100ms between 2 samples tp find cpu usage statistics */
	usleep(150000);
	/* Take the second sample regarding cpu usage statistics */
	cpu_stat_information(&second_sample);

	delta_usermode_normal_process = (second_sample.usermode_normal_process - first_sample.usermode_normal_process);
	delta_usermode_niced_process = (second_sample.usermode_niced_process - first_sample.usermode_niced_process);
	delta_kernelmode_process = (second_sample.kernelmode_process - first_sample.kernelmode_process);
	delta_idle_mode = (second_sample.idle_mode - first_sample.idle_mode);
	delta_io_completion = (second_sample.io_completion - first_sample.io_completion);
	delta_servicing_irq = (second_sample.servicing_irq - first_sample.servicing_irq);
	delta_servicing_softirq = (second_sample.servicing_softirq - first_sample.servicing_softirq);

	total_delta = delta_usermode_normal_process + delta_usermode_niced_process + delta_kernelmode_process +
                      delta_idle_mode + delta_io_completion + delta_servicing_irq + delta_servicing_softirq;

	if (total_delta != 0)
		scale = (float)100/(float)total_delta;

	f_usermode_normal_process = (float)(delta_usermode_normal_process * scale);
	f_usermode_niced_process = (float)(delta_usermode_niced_process * scale);
	f_kernelmode_process = (float)(delta_kernelmode_process * scale);
	f_idle_mode = (float)(delta_idle_mode * scale);
	f_io_completion = (float)(delta_io_completion * scale);
	f_servicing_irq = (float)(delta_servicing_irq * scale);
	f_servicing_softirq = (float)(delta_servicing_softirq * scale);

	f_usermode_normal_process = fl_round(f_usermode_normal_process);
	f_usermode_niced_process = fl_round(f_usermode_niced_process);
	f_kernelmode_process = fl_round(f_kernelmode_process);
	f_idle_mode = fl_round(f_idle_mode);
	f_io_completion = fl_round(f_io_completion);
	f_servicing_irq = fl_round(f_servicing_irq);
	f_servicing_softirq = fl_round(f_servicing_softirq);

	values[Anum_usermode_normal_process] = Float4GetDatum(f_usermode_normal_process);
	values[Anum_usermode_niced_process] = Float4GetDatum(f_usermode_niced_process);
	values[Anum_kernelmode_process] = Float4GetDatum(f_kernelmode_process);
	values[Anum_idle_mode] = Float4GetDatum(f_idle_mode);
	values[Anum_io_completion] = Float4GetDatum(f_io_completion);
	values[Anum_servicing_irq] = Float4GetDatum(f_servicing_irq);
	values[Anum_servicing_softirq] = Float4GetDatum(f_servicing_softirq);

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);
}
