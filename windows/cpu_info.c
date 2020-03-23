/*------------------------------------------------------------------------
 * cpu_info.c
 *              System CPU information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "stats.h"

#include <windows.h>
#include <wbemidl.h>

void ReadCPUInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum            values[Natts_cpu_info];
	bool             nulls[Natts_cpu_info];

	memset(nulls, 0, sizeof(nulls));

	// result code from COM calls from program
	HRESULT hres = 0;

	// Initialize COM interface pointers
	IWbemLocator         *locator = NULL;
	IWbemServices        *services = NULL;
	IEnumWbemClassObject *results = NULL;

	BSTR resource = SysAllocString(L"ROOT\\CIMV2");
	BSTR language = SysAllocString(L"WQL");
	BSTR query = SysAllocString(L"SELECT DeviceID, Caption, Manufacturer, Name, ProcessorType, Architecture,MaxClockSpeed, CurrentClockSpeed, AddressWidth, CpuStatus, L2CacheSize, L3CacheSize, NumberOfCores, NumberOfEnabledCore, NumberOfLogicalProcessors, Status, StatusInfo, ThreadCount, LastErrorCode   FROM Win32_Processor");

	// initialize COM
	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadCPUInformations]: Failed to initialize COM library")));
		return;
	}

	hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadCPUInformations]: Failed to initialize the security")));
		CoUninitialize();
		return;
	}

	/* Obtain the initial locator to Windows Management on a particular host computer */
	hres = CoCreateInstance(&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (LPVOID *)&locator);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadCPUInformations]: Failed to create IWbemLocator object")));
		CoUninitialize();
		return;
	}

	/* Connect to the root\cimv2 namespace with the current user and obtain pointer services to make IWbemServices calls */
	hres = locator->lpVtbl->ConnectServer(locator, resource, NULL, NULL, NULL, 0, NULL, NULL, &services);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadCPUInformations]: Failed to create IWbemLocator object")));
		locator->lpVtbl->Release(locator);
		CoUninitialize();
		return;
	}

	// issue a WMI query
	hres = services->lpVtbl->ExecQuery(services, language, query, WBEM_FLAG_BIDIRECTIONAL, NULL, &results);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadCPUInformations]: Failed to execute WQL query")));
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
			hres = result->lpVtbl->Get(result, L"DeviceID", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_device_id] = true;
			else
			{
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_device_id] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_device_id] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"Caption", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_description] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_description] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_description] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"Manufacturer", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_manufacturer] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_manufacturer] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_manufacturer] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"Name", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_name] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_name] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_name] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"ProcessorType", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_processor_type] = true;
			else
				values[Anum_processor_type] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"Architecture", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_architecture] = true;
			else
				values[Anum_architecture] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"MaxClockSpeed", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_max_clock_speed] = true;
			else
				values[Anum_max_clock_speed] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"CurrentClockSpeed", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_current_clock_speed] = true;
			else
				values[Anum_current_clock_speed] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"AddressWidth", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_address_width] = true;
			else
				values[Anum_address_width] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"CpuStatus", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_cpu_status] = true;
			else
				values[Anum_cpu_status] = Int32GetDatum(query_result.intVal);

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
				nulls[Anum_number_of_cores] = true;
			else
				values[Anum_number_of_cores] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"NumberOfEnabledCore", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_number_of_enabled_cores] = true;
			else
				values[Anum_number_of_enabled_cores] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"NumberOfLogicalProcessors", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_number_of_logical_processor] = true;
			else
				values[Anum_number_of_logical_processor] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"Status", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_status] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_status] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_status] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"StatusInfo", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_status_info] = true;
			else
				values[Anum_status_info] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"ThreadCount", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_thread_count] = true;
			else
				values[Anum_thread_count] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"LastErrorCode", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_last_error_code] = true;
			else
				values[Anum_last_error_code] = Int32GetDatum(query_result.intVal);

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