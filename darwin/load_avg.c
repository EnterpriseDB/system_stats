/*------------------------------------------------------------------------
 * load_avg.c
 *              System load average information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include <sys/types.h>
#include <sys/sysctl.h>

#include "postgres.h"
#include "system_stats.h"

void ReadLoadAvgInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum      values[Natts_load_avg_info];
	bool       nulls[Natts_load_avg_info];
	struct     loadavg load;
	size_t     size = sizeof(load);
	float4     load_avg_one_minute = 0;
	float4     load_avg_five_minutes = 0;
	float4     load_avg_fifteen_minutes = 0;

	memset(nulls, 0, sizeof(nulls));

	/* Read the load average from kernel */
	if (sysctlbyname("vm.loadavg", &load, &size, 0, 0) == -1)
	{
		ereport(DEBUG1, (errmsg("Error while getting loadavg information from kernel")));
		return;
	}

	load_avg_one_minute = (float4)load.ldavg[0]/load.fscale;
	load_avg_five_minutes = (float4)load.ldavg[1]/load.fscale;
	load_avg_fifteen_minutes = (float4)load.ldavg[2]/load.fscale;

	values[Anum_load_avg_one_minute]   = Float4GetDatum(load_avg_one_minute);
	values[Anum_load_avg_five_minutes] = Float4GetDatum(load_avg_five_minutes);
	values[Anum_load_avg_fifteen_minutes]  = Float4GetDatum(load_avg_fifteen_minutes);
	nulls[Anum_load_avg_ten_minutes] = true;

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);
}
