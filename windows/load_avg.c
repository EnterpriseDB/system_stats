/*------------------------------------------------------------------------
 * load_avg.c
 *              System load average information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "stats.h"

void ReadLoadAvgInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	//TODO
	// Win32_PerfFormattedData_PerfOS_System
	// SELECT * FROM Win32_PerfFormattedData_PerfOS_Processor
	// SELECT LoadPercentage FROM Win32_Processor
}
