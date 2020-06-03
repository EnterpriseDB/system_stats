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
#include <sys/sysctl.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>

struct cpu_stat
{
	uint64 user;
	uint64 system;
	uint64 idle;
	uint64 nice;
	uint64 total;
};

int cpu_stat_information(struct cpu_stat* cpu_stat);

/* Function used to get CPU state information for each mode of operation */
int cpu_stat_information(struct cpu_stat* cpu_stat)
{
	host_cpu_load_info_data_t cpu_load;
	mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
	kern_return_t ret = host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpu_load, &count);
	if (ret != KERN_SUCCESS)
	{
		ereport(DEBUG1, (errmsg("[cpu_stat_information]: Error while getting host statistics information")));
		return 1;
	}
  
	cpu_stat->user = cpu_load.cpu_ticks[CPU_STATE_USER];
	cpu_stat->system = cpu_load.cpu_ticks[CPU_STATE_SYSTEM];
	cpu_stat->idle = cpu_load.cpu_ticks[CPU_STATE_IDLE];
	cpu_stat->nice = cpu_load.cpu_ticks[CPU_STATE_NICE];
	cpu_stat->total = cpu_stat->user + cpu_stat->system + cpu_stat->idle + cpu_stat->nice;
  
	return 0;
}

void ReadCPUUsageStatistics(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum      values[Natts_cpu_usage_stats];
	bool       nulls[Natts_cpu_usage_stats];
	struct     cpu_stat first_sample, second_sample;

	memset(nulls, 0, sizeof(nulls));

	/* Take the first sample regarding cpu usage statistics */
	if (cpu_stat_information(&first_sample))
	{
		ereport(DEBUG1, (errmsg("Error while getting CPU statistics information for the first sample")));
		return;
	}

	/* sleep for the 100ms between 2 samples tp find cpu usage statistics */
	usleep(100000);
	/* Take the second sample regarding cpu usage statistics */
	if (cpu_stat_information(&second_sample))
	{
		ereport(DEBUG1, (errmsg("Error while getting CPU statistics information for the second sample")));
		return;
	}

	float4 total = (float4)(second_sample.total - first_sample.total);
	float4 user = (float4)(second_sample.user - first_sample.user) / total * 100;
	float4 system = (float4)(second_sample.system - first_sample.system) / total * 100;
	float4 idle = (float4)(second_sample.idle - first_sample.idle) / total * 100;
	float4 nice = (float4)(second_sample.nice - first_sample.nice) / total * 100;

	values[Anum_usermode_normal_process] = Float4GetDatum(user);
	values[Anum_usermode_niced_process] = Float4GetDatum(nice);
	values[Anum_kernelmode_process] = Float4GetDatum(system);
	values[Anum_idle_mode] = Float4GetDatum(idle);

	nulls[Anum_io_completion] = true;
	nulls[Anum_servicing_irq] = true;
	nulls[Anum_servicing_softirq] = true;
	nulls[Anum_percent_user_time] = true;
	nulls[Anum_percent_processor_time] = true;
	nulls[Anum_percent_privileged_time] = true;
	nulls[Anum_percent_interrupt_time] = true;

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);
}
