/*------------------------------------------------------------------------
 * cpu_memory_by_process.c
 *              CPU and memory usage by process id or name
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"

#include <windows.h>
#include <wbemidl.h>

void ReadCPUMemoryByProcess(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum            values[Natts_cpu_memory_info_by_process];
	bool             nulls[Natts_cpu_memory_info_by_process];
	MEMORYSTATUSEX   statex;
	uint64           total_physical_memory = 0;

	memset(nulls, 0, sizeof(nulls));

	statex.dwLength = sizeof(statex);

	if (GlobalMemoryStatusEx(&statex) == 0)
	{
		LPVOID lpMsgBuf;
		DWORD dw = GetLastError();
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
		ereport(DEBUG1, (errmsg("Error while getting memory information: %s", (char *)lpMsgBuf)));
		LocalFree(lpMsgBuf);
	}

	total_physical_memory = (uint64)statex.ullTotalPhys;

	// result code from COM calls from program
	HRESULT hres = 0;
	IEnumWbemClassObject *results = NULL;
	BSTR query = SysAllocString(L"SELECT * FROM Win32_PerfFormattedData_PerfProc_Process");

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

			/* Get the value from query output */
			hres = result->lpVtbl->Get(result, L"IDProcess", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_process_pid] = true;
			else
				values[Anum_process_pid] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"Name", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_process_name] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_process_name] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_process_name] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"ElapsedTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_process_running_since] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_process_running_since] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_process_running_since] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"PercentProcessorTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_percent_cpu_usage] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_percent_cpu_usage] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					float4 cpu_usage_per = (float4)val;
					values[Anum_percent_cpu_usage] = Float4GetDatum(cpu_usage_per);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"WorkingSetPrivate", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_percent_memory_usage] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_percent_memory_usage] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_process_memory_bytes] = Int64GetDatumFast(val);
					float4 memory_usage_per = (float4)(val / total_physical_memory) * 100;
					values[Anum_percent_memory_usage] = Float4GetDatum(memory_usage_per);
					free(dst);
				}
			}

			tuplestore_putvalues(tupstore, tupdesc, values, nulls);

			/* release the current result object */
			result->lpVtbl->Release(result);
		}

		/* release results set */
		results->lpVtbl->Release(results);
	}
	else
		ereport(DEBUG1, (errmsg("[ReadCPUMemoryByProcess]: Failed to get query result")));

	SysFreeString(query);
}
