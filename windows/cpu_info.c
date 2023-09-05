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

#include <windows.h>
#include <wbemidl.h>

void ReadCPUInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum            values[Natts_cpu_info];
	bool             nulls[Natts_cpu_info];
	int              no_physical_cpus = 0;

	memset(nulls, 0, sizeof(nulls));

	// result code from COM calls from program
	HRESULT hres = 0;
	IEnumWbemClassObject *results = NULL;
	BSTR query = SysAllocString(L"SELECT Caption, Manufacturer, Name, ProcessorType, Architecture,MaxClockSpeed, L2CacheSize, L3CacheSize, NumberOfCores,  NumberOfLogicalProcessors   FROM Win32_Processor");

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

			no_physical_cpus++;

			hres = result->lpVtbl->Get(result, L"Caption", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_cpu_description] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_cpu_description] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_cpu_description] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"Manufacturer", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_cpu_vendor] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_cpu_vendor] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_cpu_vendor] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"Name", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_model_name] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_model_name] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_model_name] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"ProcessorType", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_processor_type] = true;
			else
				values[Anum_processor_type] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"MaxClockSpeed", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_cpu_clock_speed] = true;
			else
			{
				/* convert MHz to Hz */
				uint64 max_clock_speed = (uint64)((uint64)(query_result.intVal) * (uint64)1000000);
				values[Anum_cpu_clock_speed] = UInt64GetDatum(max_clock_speed);
			}

			hres = result->lpVtbl->Get(result, L"Architecture", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_architecture] = true;
			else
			{
				int val = query_result.intVal;
				char arch[MAXPGPATH];
				memset(arch, 0x00, MAXPGPATH);
				sprintf(arch, "%d", val);
				values[Anum_architecture] = CStringGetTextDatum(arch);
			}

			hres = result->lpVtbl->Get(result, L"MaxClockSpeed", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_l2cache_size] = true;
			else
				values[Anum_l2cache_size] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"L2CacheSize", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_l2cache_size] = true;
			else
				values[Anum_l2cache_size] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"L3CacheSize", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_l3cache_size] = true;
			else
				values[Anum_l3cache_size] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"NumberOfCores", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_no_of_cores] = true;
			else
				values[Anum_no_of_cores] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"NumberOfLogicalProcessors", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_logical_processor] = true;
			else
				values[Anum_logical_processor] = Int32GetDatum(query_result.intVal);

			/* release the current result object */
			result->lpVtbl->Release(result);
		}

		nulls[Anum_l1dcache_size] = true;
		nulls[Anum_l1icache_size] = true;
		nulls[Anum_cpu_type] = true;
		nulls[Anum_cpu_family] = true;
		nulls[Anum_cpu_byte_order] = true;
		values[Anum_physical_processor] = Int32GetDatum(no_physical_cpus);
		tuplestore_putvalues(tupstore, tupdesc, values, nulls);

		/* release results set */
		results->lpVtbl->Release(results);
	}
	else
		ereport(DEBUG1, (errmsg("[ReadCPUInformation]: Failed to get query result")));

	SysFreeString(query);
}
