/*------------------------------------------------------------------------
 * system_stats.h
 *              Defined macros and function prototypes for system
 *              statistics information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */
#ifndef SYSTEM_STATS_H
#define SYSTEM_STATS_H

#include "access/tupdesc.h"
#include "utils/tuplestore.h"
#include "utils/builtins.h"

/* prototypes for system disk information functions */
void ReadDiskInformation(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for system IO analysis functions */
void ReadIOAnalysisInformation(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for system CPU information functions */
void ReadCPUInformation(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for system memory information functions */
void ReadMemoryInformation(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for system load average information functions */
void ReadLoadAvgInformations(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for operating system information functions */
void ReadOSInformations(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for system CPU usage information functions */
void ReadCPUUsageStatistics(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for system process information functions */
void ReadProcessInformations(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for system network information functions */
void ReadNetworkInformations(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for system network information functions */
void ReadCPUMemoryByProcess(Tuplestorestate *tupstore, TupleDesc tupdesc);

#endif // SYSTEM_STATS_H
