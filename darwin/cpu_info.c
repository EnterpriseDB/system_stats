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
    int        active_cpu = 0;
    int        byte_order = 0;
    int        cpu_family = 0;
    int        cpu_type = 0;
    int        logical_cpu = 0;
    int        physical_cpu = 0;
    uint64     cpu_frequency = 0;
    uint64     l1d_cache = 0;
    uint64     l1i_cache = 0;
    uint64     l2_cache = 0;
    uint64     l3_cache = 0;
    char       model[MAXPGPATH];
    char       machine[MAXPGPATH];
    size_t     int_size = sizeof(int);
    size_t     uint64_size = sizeof(uint64);
    size_t     char_size = sizeof(model);

	memset(nulls, 0, sizeof(nulls));
	memset(model, 0, MAXPGPATH);
	memset(machine, 0, MAXPGPATH);


    if (sysctlbyname("hw.activecpu", &active_cpu, &int_size, 0, 0) == -1)
        nulls[Anum_active_cpu] = true;

    if (sysctlbyname("hw.byteorder", &byte_order, &int_size, 0, 0) == -1)
        nulls[Anum_byte_order] = true;

    if (sysctlbyname("hw.cpufamily", &cpu_family, &int_size, 0, 0) == -1)
        nulls[Anum_cpu_family] = true;

    if (sysctlbyname("hw.cputype", &cpu_type, &int_size, 0, 0) == -1)
        nulls[Anum_cpu_type] = true;

    if (sysctlbyname("hw.logicalcpu", &logical_cpu, &int_size, 0, 0) == -1)
        nulls[Anum_logical_cpu] = true;

    if (sysctlbyname("hw.physicalcpu", &physical_cpu, &int_size, 0, 0) == -1)
        nulls[Anum_physical_cpu] = true;

    if (sysctlbyname("hw.cpufrequency", &cpu_frequency, &uint64_size, 0, 0) == -1)
        nulls[Anum_cpu_freq_hz] = true;

    if (sysctlbyname("hw.l1dcachesize", &l1d_cache, &uint64_size, 0, 0) == -1)
        nulls[Anum_l1dcache_size] = true;

    if (sysctlbyname("hw.l1icachesize", &l1i_cache, &uint64_size, 0, 0) == -1)
        nulls[Anum_l1icache_size] = true;

    if (sysctlbyname("hw.l2cachesize", &l2_cache, &uint64_size, 0, 0) == -1)
        nulls[Anum_l2cache_size] = true;

    if (sysctlbyname("hw.l3cachesize", &l3_cache, &uint64_size, 0, 0) == -1)
        nulls[Anum_l3cache_size] = true;

    if (sysctlbyname("hw.model", model, &char_size, NULL, 0) == -1)
        nulls[Anum_cpu_model] = true;

    char_size = sizeof(machine);
    if (sysctlbyname("hw.machine", machine, &char_size, 0, 0) == -1)
        nulls[Anum_cpu_machine] = true;

    values[Anum_active_cpu] = Int32GetDatum(active_cpu);
    values[Anum_byte_order] = Int32GetDatum(byte_order);
    values[Anum_cpu_family] = Int32GetDatum(cpu_family);
    values[Anum_cpu_type] = Int32GetDatum(cpu_type);
    values[Anum_logical_cpu] = Int32GetDatum(logical_cpu);
    values[Anum_physical_cpu] = Int32GetDatum(physical_cpu);
    values[Anum_cpu_freq_hz] = Int64GetDatumFast(cpu_frequency);
    values[Anum_l1dcache_size] = Int64GetDatumFast(l1d_cache);
    values[Anum_l1icache_size] = Int64GetDatumFast(l1i_cache);
    values[Anum_l2cache_size] = Int64GetDatumFast(l2_cache);
    values[Anum_l3cache_size] = Int64GetDatumFast(l3_cache);
    values[Anum_cpu_model] = CStringGetTextDatum(model);
    values[Anum_cpu_machine] = CStringGetTextDatum(machine);

    tuplestore_putvalues(tupstore, tupdesc, values, nulls);
}
