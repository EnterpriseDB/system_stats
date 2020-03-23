/*------------------------------------------------------------------------
 * system_stats.c
 *              System statistics information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>

#include "postgres.h"
#include "system_stats.h"

#include "catalog/pg_type.h"
#include "datatype/timestamp.h"
#include "fmgr.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "nodes/execnodes.h"
#include "nodes/nodes.h"
#include "pgstat.h"
#include "port.h"
#include "storage/fd.h"
#include "utils/timestamp.h"

PG_MODULE_MAGIC;

/* called when extension is loaded */
void _PG_init(void);

/* called when extension is unloaded */
void _PG_fini(void);

#ifdef WIN32
/* Add a prototype marked PGDLLEXPORT */
PGDLLEXPORT Datum pg_sys_os_info(PG_FUNCTION_ARGS);
PGDLLEXPORT Datum pg_sys_cpu_info(PG_FUNCTION_ARGS);
PGDLLEXPORT Datum pg_sys_memory_info(PG_FUNCTION_ARGS);
PGDLLEXPORT Datum pg_sys_cpu_usage_info(PG_FUNCTION_ARGS);
PGDLLEXPORT Datum pg_sys_load_avg_info(PG_FUNCTION_ARGS);
PGDLLEXPORT Datum pg_sys_io_analysis_info(PG_FUNCTION_ARGS);
PGDLLEXPORT Datum pg_sys_disk_info(PG_FUNCTION_ARGS);
PGDLLEXPORT Datum pg_sys_process_info(PG_FUNCTION_ARGS);
PGDLLEXPORT Datum pg_sys_network_info(PG_FUNCTION_ARGS);
PGDLLEXPORT Datum pg_sys_cpu_memory_by_process(PG_FUNCTION_ARGS);
#endif

PG_FUNCTION_INFO_V1(pg_sys_os_info);
PG_FUNCTION_INFO_V1(pg_sys_cpu_info);
PG_FUNCTION_INFO_V1(pg_sys_memory_info);
PG_FUNCTION_INFO_V1(pg_sys_cpu_usage_info);
PG_FUNCTION_INFO_V1(pg_sys_load_avg_info);
PG_FUNCTION_INFO_V1(pg_sys_io_analysis_info);
PG_FUNCTION_INFO_V1(pg_sys_disk_info);
PG_FUNCTION_INFO_V1(pg_sys_process_info);
PG_FUNCTION_INFO_V1(pg_sys_network_info);
PG_FUNCTION_INFO_V1(pg_sys_cpu_memory_by_process);

void _PG_init(void)
{
	/* loading system stats extension */
	elog(DEBUG1, "System stats extension loaded...");
}

void _PG_fini(void)
{
	/* unloading system stats extension */
	elog(DEBUG1, "System stats extension unloaded...");
}

/*
 * pg_sys_os_info
 *
 * This function will give operating system and kernel information
 *
 */
Datum
pg_sys_os_info(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	/*
	 * Tuple descriptor describing the result of operating system information
	 */
	TupleDesc       tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;

	// check to see if caller supports us returning a tuplestore
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("set-valued function called in context that cannot accept a set")));

	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("materialize mode required, but it is not allowed in this context")));

	// Switch into long-lived context to construct returned data structures
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	// Build a tuple descriptor for our result type
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	Assert(tupdesc->natts == Natts_os_info);

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	/* Fetch the Operating system information and put in tuple store */
	ReadOSInformations(tupstore, tupdesc);

	tuplestore_donestoring(tupstore);

	return (Datum) 0;
}

/*
 * pg_sys_cpu_info
 *
 * This function will give system CPU information
 *
 */
Datum
pg_sys_cpu_info(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;

	/*
	 * Tuple descriptor describing the result of CPU information of the system
	 */
	TupleDesc       tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;

	// check to see if caller supports us returning a tuplestore
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("set-valued function called in context that cannot accept a set")));

	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("materialize mode required, but it is not allowed in this context")));

	// Switch into long-lived context to construct returned data structures
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	// Build a tuple descriptor for our result type
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	Assert(tupdesc->natts == Natts_cpu_info);

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	/* Fetch the system CPU information and put in tuple store */
	ReadCPUInformation(tupstore, tupdesc);

	tuplestore_donestoring(tupstore);

	return (Datum) 0;
}

/*
 * pg_sys_memory_info
 *
 * This function will give system memory information
 *
 */
Datum
pg_sys_memory_info(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	/*
	 * Tuple descriptor describing the result of memory information
	 */
	TupleDesc       tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;

	// check to see if caller supports us returning a tuplestore
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("set-valued function called in context that cannot accept a set")));

	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("materialize mode required, but it is not allowed in this context")));

	// Switch into long-lived context to construct returned data structures
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	// Build a tuple descriptor for our result type
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	Assert(tupdesc->natts == Natts_memory_info);

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	/* Fetch the system memory information and put in tuple store */
	ReadMemoryInformation(tupstore, tupdesc);

	tuplestore_donestoring(tupstore);

	return (Datum) 0;
}

/*
 * pg_sys_io_analysis_info
 *
 * This function will give system IO analysis information
 *
 */
Datum
pg_sys_io_analysis_info(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	/*
	 * Tuple descriptor describing the result of system io analysis information
	 */
	TupleDesc       tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;

	// check to see if caller supports us returning a tuplestore
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("set-valued function called in context that cannot accept a set")));

	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("materialize mode required, but it is not allowed in this context")));

	// Switch into long-lived context to construct returned data structures
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	// Build a tuple descriptor for our result type
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	Assert(tupdesc->natts == Natts_io_analysis_info);

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	/* Fetch the system io analysis information and put in tuple store */
	ReadIOAnalysisInformation(tupstore, tupdesc);

	tuplestore_donestoring(tupstore);

	return (Datum) 0;
}

/*
 * pg_sys_disk_info
 *
 * This function will give system disk information
 *
 */
Datum
pg_sys_disk_info(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	/*
	 * Tuple descriptor describing the result of system disk information
	 */
	TupleDesc       tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;

	// check to see if caller supports us returning a tuplestore
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("set-valued function called in context that cannot accept a set")));

	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("materialize mode required, but it is not allowed in this context")));

	// Switch into long-lived context to construct returned data structures
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	// Build a tuple descriptor for our result type
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	Assert(tupdesc->natts == Natts_disk_info);

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	/* Fetch the system disk information and put in tuple store */
	ReadDiskInformation(tupstore, tupdesc);

	tuplestore_donestoring(tupstore);

	return (Datum) 0;
}

/*
 * pg_sys_load_avg_info
 *
 * This function will give system load average information
 *
 */
Datum
pg_sys_load_avg_info(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	/*
	 * Tuple descriptor describing the result of system load average information
	 */
	TupleDesc       tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;

	// check to see if caller supports us returning a tuplestore
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("set-valued function called in context that cannot accept a set")));

	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("materialize mode required, but it is not allowed in this context")));

	// Switch into long-lived context to construct returned data structures
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	// Build a tuple descriptor for our result type
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	Assert(tupdesc->natts == Natts_load_avg_info);

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	/* Fetch the system load average information and put in tuple store */
	ReadLoadAvgInformations(tupstore, tupdesc);

	tuplestore_donestoring(tupstore);

	return (Datum) 0;
}


/*
 * pg_sys_cpu_usage_info
 *
 * This function will give system CPU usage information
 *
 */
Datum
pg_sys_cpu_usage_info(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	/*
	 * Tuple descriptor describing the result of cpu usage information
	 */
	TupleDesc       tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;

	// check to see if caller supports us returning a tuplestore
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("set-valued function called in context that cannot accept a set")));

	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("materialize mode required, but it is not allowed in this context")));

	// Switch into long-lived context to construct returned data structures
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	// Build a tuple descriptor for our result type
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	Assert(tupdesc->natts == Natts_cpu_usage_stats);

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	/* Fetch the system CPU usage information and put in tuple store */
	ReadCPUUsageStatistics(tupstore, tupdesc);

	tuplestore_donestoring(tupstore);

	return (Datum) 0;
}

/*
 * pg_sys_process_info
 *
 * This function will give system process information
 *
 */
Datum
pg_sys_process_info(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	/*
	 * Tuple descriptor describing the result of process information
	 */
	TupleDesc       tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;

	// check to see if caller supports us returning a tuplestore
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("set-valued function called in context that cannot accept a set")));

	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("materialize mode required, but it is not allowed in this context")));

	// Switch into long-lived context to construct returned data structures
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	// Build a tuple descriptor for our result type
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	Assert(tupdesc->natts == Natts_process_info);

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	/* Fetch the system process information and put in tuple store */
	ReadProcessInformations(tupstore, tupdesc);

	tuplestore_donestoring(tupstore);

	return (Datum) 0;
}

/*
 * pg_sys_network_info
 *
 * This function will give system network information
 *
 */
Datum
pg_sys_network_info(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	/*
	 * Tuple descriptor describing the result of network information
	 */
	TupleDesc       tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;

	// check to see if caller supports us returning a tuplestore
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("set-valued function called in context that cannot accept a set")));

	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("materialize mode required, but it is not allowed in this context")));

	// Switch into long-lived context to construct returned data structures
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	// Build a tuple descriptor for our result type
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	Assert(tupdesc->natts == Natts_network_info);

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	/* Fetch the system network information and put in tuple store */
	ReadNetworkInformations(tupstore, tupdesc);

	tuplestore_donestoring(tupstore);

	return (Datum) 0;
}

/*
 * pg_sys_cpu_memory_by_process
 *
 * This function will give cpu and memory usage by process ID
 *
 */
Datum
pg_sys_cpu_memory_by_process(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	/*
	 * Tuple descriptor describing the result of network information
	 */
	TupleDesc       tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext   per_query_ctx;
	MemoryContext   oldcontext;

	// check to see if caller supports us returning a tuplestore
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("set-valued function called in context that cannot accept a set")));

	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("materialize mode required, but it is not allowed in this context")));

	// Switch into long-lived context to construct returned data structures
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	// Build a tuple descriptor for our result type
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	Assert(tupdesc->natts == Natts_cpu_memory_info_by_process);

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	/* Fetch the system cpu and memory usage by process */
	ReadCPUMemoryByProcess(tupstore, tupdesc);

	tuplestore_donestoring(tupstore);

	return (Datum) 0;
}
