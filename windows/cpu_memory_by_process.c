/*------------------------------------------------------------------------
 * cpu_memory_by_process.c
 *              CPU and memory usage by process id or name
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "stats.h"

#include <windows.h>
#include <wbemidl.h>

void ReadCPUMemoryByProcess(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum            values[Natts_cpu_memory_info_by_process];
	bool             nulls[Natts_cpu_memory_info_by_process];

	memset(nulls, 0, sizeof(nulls));

	// result code from COM calls from program
	HRESULT hres = 0;

	// Initialize COM interface pointers
	IWbemLocator         *locator = NULL;
	IWbemServices        *services = NULL;
	IEnumWbemClassObject *results = NULL;

	BSTR resource = SysAllocString(L"ROOT\\CIMV2");
	BSTR language = SysAllocString(L"WQL");
	BSTR query = SysAllocString(L"SELECT * FROM Win32_PerfFormattedData_PerfProc_Process");

	// initialize COM
	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadCPUMemoryByProcess]: Failed to initialize COM library")));
		return;
	}

	hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadCPUMemoryByProcess]: Failed to initialize the security")));
		CoUninitialize();
		return;
	}

	/* Obtain the initial locator to Windows Management on a particular host computer */
	hres = CoCreateInstance(&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (LPVOID *)&locator);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadCPUMemoryByProcess]: Failed to create IWbemLocator object")));
		CoUninitialize();
		return;
	}

	/* Connect to the root\cimv2 namespace with the current user and obtain pointer services to make IWbemServices calls */
	hres = locator->lpVtbl->ConnectServer(locator, resource, NULL, NULL, NULL, 0, NULL, NULL, &services);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadCPUMemoryByProcess]: Failed to create IWbemLocator object")));
		locator->lpVtbl->Release(locator);
		CoUninitialize();
		return;
	}

	// issue a WMI query
	hres = services->lpVtbl->ExecQuery(services, language, query, WBEM_FLAG_BIDIRECTIONAL, NULL, &results);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadCPUMemoryByProcess]: Failed to execute WQL query")));
		services->lpVtbl->Release(services);
		locator->lpVtbl->Release(locator);
		CoUninitialize();
		return;
	}

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

			hres = result->lpVtbl->Get(result, L"ThreadCount", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_process_thread_count] = true;
			else
				values[Anum_process_thread_count] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"HandleCount", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_process_handle_count] = true;
			else
				values[Anum_process_handle_count] = Int32GetDatum(query_result.intVal);

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
					long long val = strtoll(dst, NULL, 10);
					values[Anum_percent_processor_time] = Int64GetDatumFast(val);
					free(dst);
				}
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
					long long val = strtoll(dst, NULL, 10);
					values[Anum_percent_user_time] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"WorkingSetPrivate", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_memory_usage_bytes] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_memory_usage_bytes] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_memory_usage_bytes] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			tuplestore_putvalues(tupstore, tupdesc, values, nulls);

			/* release the current result object */
			result->lpVtbl->Release(result);
		}
	}

	/* release WMI COM interfaces */
	results->lpVtbl->Release(results);
	services->lpVtbl->Release(services);
	locator->lpVtbl->Release(locator);

	CoUninitialize();

	SysFreeString(query);
	SysFreeString(language);
	SysFreeString(resource);
}
