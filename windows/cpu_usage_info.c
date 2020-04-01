/*------------------------------------------------------------------------
 * cpu_usage_info.c
 *              System CPU usage information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "stats.h"

#include <windows.h>
#include <wbemidl.h>

void ReadCPUUsageStatistics(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum            values[Natts_cpu_usage_info];
	bool             nulls[Natts_cpu_usage_info];

	memset(nulls, 0, sizeof(nulls));

	// result code from COM calls from program
	HRESULT hres = 0;
	IEnumWbemClassObject *results = NULL;
	BSTR query = SysAllocString(L"SELECT * FROM Win32_PerfFormattedData_PerfOS_Processor");

	// issue a WMI query
	results = execute_query(query);

	/* list the query results */
	if (results != NULL)
	{
		IWbemClassObject *result = NULL;
		ULONG returnedCount = 0;

		// enumerate the retrieved objects
		while ((hres = results->lpVtbl->Next(results, WBEM_INFINITE, 1, &result, &returnedCount)) == S_OK)
		{
			VARIANT query_result;

			int     wstr_length = 0;
			char    *dst = NULL;
			size_t  charsConverted = 0;

			hres = result->lpVtbl->Get(result, L"Name", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_cpu_usage_name] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_cpu_usage_name] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_cpu_usage_name] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"Caption", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_cpu_usage_caption] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_cpu_usage_caption] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_cpu_usage_caption] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"Description", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_cpu_usage_description] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_cpu_usage_description] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_cpu_usage_description] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"PercentIdleTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_cpu_percent_idle_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_cpu_percent_idle_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long percent_time = strtoll(dst, NULL, 10);
					values[Anum_cpu_percent_idle_time] = Int64GetDatumFast(percent_time);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"PercentInterruptTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_cpu_percent_interrupt_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_cpu_percent_interrupt_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long percent_time = strtoll(dst, NULL, 10);
					values[Anum_cpu_percent_interrupt_time] = Int64GetDatumFast(percent_time);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"PercentPrivilegedTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_cpu_percent_privileged_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_cpu_percent_privileged_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long percent_time = strtoll(dst, NULL, 10);
					values[Anum_cpu_percent_privileged_time] = Int64GetDatumFast(percent_time);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"PercentProcessorTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_cpu_percent_processor_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_cpu_percent_processor_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long percent_time = strtoll(dst, NULL, 10);
					values[Anum_cpu_percent_processor_time] = Int64GetDatumFast(percent_time);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"PercentUserTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_cpu_percent_user_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_cpu_percent_user_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long percent_time = strtoll(dst, NULL, 10);
					values[Anum_cpu_percent_user_time] = Int64GetDatumFast(percent_time);
					free(dst);
				}
			}

			tuplestore_putvalues(tupstore, tupdesc, values, nulls);

			/* release the current result object */
			result->lpVtbl->Release(result);
		}
	}
	else
		ereport(DEBUG1, (errmsg("[ReadCPUUsageStatistics]: Failed to get query result")));

	/* release WMI COM interfaces */
	results->lpVtbl->Release(results);
	SysFreeString(query);
}
