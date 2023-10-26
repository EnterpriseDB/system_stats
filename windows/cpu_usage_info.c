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

#include <windows.h>
#include <wbemidl.h>

void ReadCPUUsageStatistics(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum            values[Natts_cpu_usage_stats];
	bool             nulls[Natts_cpu_usage_stats];

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

			hres = result->lpVtbl->Get(result, L"PercentIdleTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_idle_mode] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_idle_mode] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long percent_time = strtoll(dst, NULL, 10);
					values[Anum_idle_mode] = Float4GetDatum((float)percent_time);
					free(dst);
				}
				VariantClear(&query_result);
			}

			hres = result->lpVtbl->Get(result, L"PercentInterruptTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_percent_interrupt_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_percent_interrupt_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long percent_time = strtoll(dst, NULL, 10);
					values[Anum_percent_interrupt_time] = Float4GetDatum((float)percent_time);
					free(dst);
				}
				VariantClear(&query_result);
			}

			hres = result->lpVtbl->Get(result, L"PercentPrivilegedTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_percent_privileged_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_percent_privileged_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long percent_time = strtoll(dst, NULL, 10);
					values[Anum_percent_privileged_time] = Float4GetDatum((float)percent_time);
					free(dst);
				}
				VariantClear(&query_result);
			}

			hres = result->lpVtbl->Get(result, L"PercentProcessorTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_percent_processor_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_percent_processor_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long percent_time = strtoll(dst, NULL, 10);
					values[Anum_percent_processor_time] = Float4GetDatum((float)percent_time);
					free(dst);
				}
				VariantClear(&query_result);
			}

			hres = result->lpVtbl->Get(result, L"PercentUserTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_percent_user_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_percent_user_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long percent_time = strtoll(dst, NULL, 10);
					values[Anum_percent_user_time] = Float4GetDatum((float)percent_time);
					free(dst);
				}
				VariantClear(&query_result);
			}

			nulls[Anum_usermode_normal_process] = true;
			nulls[Anum_usermode_niced_process] = true;
			nulls[Anum_kernelmode_process] = true;
			nulls[Anum_io_completion] = true;
			nulls[Anum_servicing_irq] = true;
			nulls[Anum_servicing_softirq] = true;

			tuplestore_putvalues(tupstore, tupdesc, values, nulls);

			/* release the current result object */
			result->lpVtbl->Release(result);
			break;
		}

		/* release results set */
		results->lpVtbl->Release(results);
	}
	else
		ereport(DEBUG1, (errmsg("[ReadCPUUsageStatistics]: Failed to get query result")));

	SysFreeString(query);
}
