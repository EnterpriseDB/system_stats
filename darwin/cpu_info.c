/*------------------------------------------------------------------------
 * cpu_info.c
 *              System CPU information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"

#include <sys/types.h>
#include <sys/sysctl.h>

void ReadCPUInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum      values[Natts_cpu_info];
	bool       nulls[Natts_cpu_info];
	int        byte_order = 0;
	int        cpu_family = 0;
	int        cpu_type = 0;
	int        no_of_cores = 0;
	int        logical_cpu = 0;
	int        physical_cpu = 0;
	uint64     cpu_frequency = 0;
	uint64     l1d_cache_bytes = 0;
	uint64     l1i_cache_bytes = 0;
	uint64     l2_cache_bytes = 0;
	uint64     l3_cache_bytes = 0;
	char       model[MAXPGPATH];
	char       machine[MAXPGPATH];
	char       s_byte_order[MAXPGPATH];
	char       s_cpu_family[MAXPGPATH];
	char       s_cpu_type[MAXPGPATH];
	size_t     int_size = sizeof(int);
	size_t     uint64_size = sizeof(uint64);
	size_t     char_size = sizeof(model);
	int num_lst[2];
	size_t len = 4;
	uint32_t count;

	memset(nulls, 0, sizeof(nulls));
	memset(model, 0, MAXPGPATH);
	memset(machine, 0, MAXPGPATH);
	memset(s_byte_order, 0, MAXPGPATH);
	memset(s_cpu_family, 0, MAXPGPATH);
	memset(s_cpu_type, 0, MAXPGPATH);

	num_lst[0] = CTL_HW; num_lst[1] = HW_AVAILCPU;
	sysctl(num_lst, 2, &count, &len, NULL, 0);

	if(count < 1)
	{
		num_lst[1] = HW_NCPU;
		sysctl(num_lst, 2, &count, &len, NULL, 0);
		if(count < 1)
			count = 1;
	}

	no_of_cores = count;

	if (sysctlbyname("hw.byteorder", &byte_order, &int_size, 0, 0) == -1)
		nulls[Anum_cpu_byte_order] = true;

	if (sysctlbyname("hw.cpufamily", &cpu_family, &int_size, 0, 0) == -1)
		nulls[Anum_cpu_family] = true;

	if (sysctlbyname("hw.cputype", &cpu_type, &int_size, 0, 0) == -1)
		nulls[Anum_cpu_type] = true;

	if (sysctlbyname("hw.logicalcpu", &logical_cpu, &int_size, 0, 0) == -1)
		nulls[Anum_logical_processor] = true;

	if (sysctlbyname("hw.physicalcpu", &physical_cpu, &int_size, 0, 0) == -1)
		nulls[Anum_physical_processor] = true;

	if (sysctlbyname("hw.cpufrequency", &cpu_frequency, &uint64_size, 0, 0) == -1)
		nulls[Anum_cpu_clock_speed] = true;

	if (sysctlbyname("hw.l1dcachesize", &l1d_cache_bytes, &uint64_size, 0, 0) == -1)
		nulls[Anum_l1dcache_size] = true;

	if (sysctlbyname("hw.l1icachesize", &l1i_cache_bytes, &uint64_size, 0, 0) == -1)
		nulls[Anum_l1icache_size] = true;

	if (sysctlbyname("hw.l2cachesize", &l2_cache_bytes, &uint64_size, 0, 0) == -1)
		nulls[Anum_l2cache_size] = true;

	if (sysctlbyname("hw.l3cachesize", &l3_cache_bytes, &uint64_size, 0, 0) == -1)
		nulls[Anum_l3cache_size] = true;

	if (sysctlbyname("hw.model", model, &char_size, NULL, 0) == -1)
		nulls[Anum_model_name] = true;

	char_size = sizeof(machine);
	if (sysctlbyname("hw.machine", machine, &char_size, 0, 0) == -1)
		nulls[Anum_architecture] = true;

	sprintf(s_byte_order, "%d", byte_order);
	sprintf(s_cpu_family, "%d", cpu_family);
	sprintf(s_cpu_type, "%d", cpu_type);

	nulls[Anum_cpu_vendor] = true;
	nulls[Anum_cpu_description] = true;
	nulls[Anum_processor_type] = true;

	values[Anum_no_of_cores] = Int32GetDatum(no_of_cores);
	values[Anum_cpu_byte_order] = CStringGetTextDatum(s_byte_order);
	values[Anum_cpu_family] = CStringGetTextDatum(s_cpu_family);
	values[Anum_cpu_type] = CStringGetTextDatum(s_cpu_type);
	values[Anum_logical_processor] = Int32GetDatum(logical_cpu);
	values[Anum_physical_processor] = Int32GetDatum(physical_cpu);
	values[Anum_cpu_clock_speed] = Int64GetDatumFast(cpu_frequency);
	values[Anum_l1dcache_size] = Int32GetDatum((int)(l1d_cache_bytes/1024));
	values[Anum_l1icache_size] = Int32GetDatum((int)(l1i_cache_bytes/1024));
	values[Anum_l2cache_size] = Int32GetDatum((int)(l2_cache_bytes/1024));
	values[Anum_l3cache_size] = Int32GetDatum((int)(l3_cache_bytes/1024));
	values[Anum_model_name] = CStringGetTextDatum(model);
	values[Anum_architecture] = CStringGetTextDatum(machine);

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);
}
