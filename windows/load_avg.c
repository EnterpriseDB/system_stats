/*------------------------------------------------------------------------
 * load_avg.c
 *              System load average information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"

#include <windows.h>
#include <pdh.h>

#pragma comment(lib, "Pdh.lib")

/* logic will be same as linux */
#define SAMPLING_INTERVAL 5
/* This has been calculated as 1/exp(5/60) , As 5 is sampling interval 5 second and 60 is for load avg of 1 minute ( 60 Seconds ) */
#define LOAD_AVG_1F  0.920044
/* This has been calculated as 1/exp(5/300) , As 5 is sampling interval 5 second and for load avg of 5 minutes ( 300 Seconds ) */
#define LOAD_AVG_5F  0.983471
/* This has been calculated as 1/exp(5/900) , As 5 is sampling interval 5 second and for load avg of 15 minutes ( 900 Seconds ) */
#define LOAD_AVG_15F 0.994459

bool load_avg_initialized = false;
double load_avg_1m  = 0;
double load_avg_5m  = 0;
double load_avg_10m = 0;
double load_avg_15m = 0;

VOID CALLBACK LoadAvgCallback(PVOID hCounter)
{
	PDH_FMT_COUNTERVALUE displayValue;
	double currentLoad;
	PDH_STATUS err;

	err = PdhGetFormattedCounterValue(
		(PDH_HCOUNTER)hCounter, PDH_FMT_DOUBLE, 0, &displayValue);
	// Skip updating the load if we can't get the value successfully
	if (err != ERROR_SUCCESS) {
		return;
	}

	currentLoad = displayValue.doubleValue;

	load_avg_1m = load_avg_1m * LOAD_AVG_1F + currentLoad * (1.0 - LOAD_AVG_1F);
	load_avg_5m = load_avg_5m * LOAD_AVG_5F + currentLoad * (1.0 - LOAD_AVG_5F);
	load_avg_15m = load_avg_15m * LOAD_AVG_15F + currentLoad * (1.0 - LOAD_AVG_15F);
}

void initialize_load_avg_info()
{
	WCHAR *szCounterPath = L"\\System\\Processor Queue Length";
	PDH_STATUS s;
	BOOL ret;
	HQUERY hQuery;
	HCOUNTER hCounter;
	HANDLE event;
	HANDLE waitHandle;

	if ((PdhOpenQueryW(NULL, 0, &hQuery)) != ERROR_SUCCESS)
	{
		ereport(DEBUG1, (errmsg("[initialize_load_avg_info]: PdhOpenQueryW is failed")));
		return;
	}

	s = PdhAddEnglishCounterW(hQuery, szCounterPath, 0, &hCounter);
	if (s != ERROR_SUCCESS)
	{
		ereport(DEBUG1, (errmsg("[initialize_load_avg_info]: PdhAddEnglishCounterW is failed")));
		return;
	}

	event = CreateEventW(NULL, FALSE, FALSE, L"LoadUpdateEvent");
	if (event == NULL)
	{
		ereport(DEBUG1, (errmsg("[initialize_load_avg_info]: CreateEventW is failed")));
		return;
	}

	s = PdhCollectQueryDataEx(hQuery, SAMPLING_INTERVAL, event);
	if (s != ERROR_SUCCESS)
	{
		ereport(DEBUG1, (errmsg("[initialize_load_avg_info]: PdhCollectQueryDataEx is failed")));
		return;
	}

	ret = RegisterWaitForSingleObject(
		&waitHandle,
		event,
		(WAITORTIMERCALLBACK)LoadAvgCallback,
		(PVOID)
		hCounter,
		INFINITE,
		WT_EXECUTEDEFAULT);

	if (ret == 0)
	{
		ereport(DEBUG1, (errmsg("[initialize_load_avg_info]: RegisterWaitForSingleObject is failed")));
		return;
	}
}

void ReadLoadAvgInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum      values[Natts_load_avg_info];
	bool       nulls[Natts_load_avg_info];

	memset(nulls, 0, sizeof(nulls));

	/* InitialiZe the count to get the samples */
	if (!load_avg_initialized)
	{
		ereport(DEBUG1, (errmsg("[ReadLoadAvgInformations]: Initializing counter for load average")));
		initialize_load_avg_info();
		load_avg_initialized = true;
	}
	else
		ereport(DEBUG1, (errmsg("[ReadLoadAvgInformations]: Counter already initializing for load average")));

	values[Anum_load_avg_one_minute]      = Float4GetDatum((float4)load_avg_1m);
	values[Anum_load_avg_five_minutes]    = Float4GetDatum((float4)load_avg_5m);
	values[Anum_load_avg_ten_minutes]     = Float4GetDatum((float4)load_avg_10m);
	values[Anum_load_avg_fifteen_minutes] = Float4GetDatum((float4)load_avg_15m);

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);
}
