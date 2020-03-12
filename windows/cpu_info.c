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
			VARIANT device_id, caption, manufacturer, name, processor_type, architecture,
				max_clock_speed, current_clock_speed, address_width, cpu_status, l2cache_size,
				l3cache_size, no_of_cores, no_of_enabled_cores, no_of_logical_processor,
				status, status_info, thread_count, last_error_code;
			int     wstr_length = 0;
			char    *dst = NULL;
			size_t  charsConverted = 0;

			/* Get the value from query output */
			hres = result->lpVtbl->Get(result, L"DeviceID", 0, &device_id, 0, 0);
			hres = result->lpVtbl->Get(result, L"Caption", 0, &caption, 0, 0);
			hres = result->lpVtbl->Get(result, L"Manufacturer", 0, &manufacturer, 0, 0);
			hres = result->lpVtbl->Get(result, L"Name", 0, &name, 0, 0);
			hres = result->lpVtbl->Get(result, L"ProcessorType", 0, &processor_type, 0, 0);
			hres = result->lpVtbl->Get(result, L"Architecture", 0, &architecture, 0, 0);
			hres = result->lpVtbl->Get(result, L"MaxClockSpeed", 0, &max_clock_speed, 0, 0);
			hres = result->lpVtbl->Get(result, L"CurrentClockSpeed", 0, &current_clock_speed, 0, 0);
			hres = result->lpVtbl->Get(result, L"AddressWidth", 0, &address_width, 0, 0);
			hres = result->lpVtbl->Get(result, L"CpuStatus", 0, &cpu_status, 0, 0);
			hres = result->lpVtbl->Get(result, L"L2CacheSize", 0, &l2cache_size, 0, 0);
			hres = result->lpVtbl->Get(result, L"L3CacheSize", 0, &l3cache_size, 0, 0);
			hres = result->lpVtbl->Get(result, L"NumberOfCores", 0, &no_of_cores, 0, 0);
			hres = result->lpVtbl->Get(result, L"NumberOfEnabledCore", 0, &no_of_enabled_cores, 0, 0);
			hres = result->lpVtbl->Get(result, L"NumberOfLogicalProcessors", 0, &no_of_logical_processor, 0, 0);
			hres = result->lpVtbl->Get(result, L"Status", 0, &status, 0, 0);
			hres = result->lpVtbl->Get(result, L"StatusInfo", 0, &status_info, 0, 0);
			hres = result->lpVtbl->Get(result, L"ThreadCount", 0, &thread_count, 0, 0);
			hres = result->lpVtbl->Get(result, L"LastErrorCode", 0, &last_error_code, 0, 0);

			wstr_length = SysStringLen(device_id.bstrVal);
			if (wstr_length == 0)
				nulls[Anum_device_id] = true;
			else
			{
				dst = (char *)malloc(wstr_length + 10);
				memset(dst, 0x00, (wstr_length + 10));
				wcstombs_s(&charsConverted, dst, wstr_length + 10, device_id.bstrVal, wstr_length);
				values[Anum_device_id] = CStringGetTextDatum(dst);
				free(dst);
			}

			wstr_length = 0;
			charsConverted = 0;
			wstr_length = SysStringLen(caption.bstrVal);
			if (wstr_length == 0)
				nulls[Anum_description] = true;
			else
			{
				dst = (char *)malloc(wstr_length + 10);
				memset(dst, 0x00, (wstr_length + 10));
				wcstombs_s(&charsConverted, dst, wstr_length + 10, caption.bstrVal, wstr_length);
				values[Anum_description] = CStringGetTextDatum(dst);
				free(dst);
			}

			wstr_length = 0;
			charsConverted = 0;
			wstr_length = SysStringLen(manufacturer.bstrVal);
			if (wstr_length == 0)
				nulls[Anum_manufacturer] = true;
			else
			{
				dst = (char *)malloc(wstr_length + 10);
				memset(dst, 0x00, (wstr_length + 10));
				wcstombs_s(&charsConverted, dst, wstr_length + 10, manufacturer.bstrVal, wstr_length);
				values[Anum_manufacturer] = CStringGetTextDatum(dst);
				free(dst);
			}

			wstr_length = 0;
			charsConverted = 0;
			wstr_length = SysStringLen(name.bstrVal);
			if (wstr_length == 0)
				nulls[Anum_name] = true;
			else
			{
				dst = (char *)malloc(wstr_length + 10);
				memset(dst, 0x00, (wstr_length + 10));
				wcstombs_s(&charsConverted, dst, wstr_length + 10, name.bstrVal, wstr_length);
				values[Anum_name] = CStringGetTextDatum(dst);
				free(dst);
			}

			wstr_length = 0;
			charsConverted = 0;
			wstr_length = SysStringLen(status.bstrVal);
			if (wstr_length == 0)
				nulls[Anum_status] = true;
			else
			{
				dst = (char *)malloc(wstr_length + 10);
				memset(dst, 0x00, (wstr_length + 10));
				wcstombs_s(&charsConverted, dst, wstr_length + 10, status.bstrVal, wstr_length);
				values[Anum_status] = CStringGetTextDatum(dst);
				free(dst);
			}

			values[Anum_processor_type] = Int32GetDatum(processor_type.intVal);
			values[Anum_architecture] = Int32GetDatum(architecture.intVal);
			values[Anum_max_clock_speed] = Int32GetDatum(max_clock_speed.intVal);
			values[Anum_current_clock_speed] = Int32GetDatum(current_clock_speed.intVal);
			values[Anum_address_width] = Int32GetDatum(address_width.intVal);
			values[Anum_cpu_status] = Int32GetDatum(cpu_status.intVal);
			values[Anum_l2cache_size] = Int32GetDatum(l2cache_size.intVal);
			values[Anum_l3cache_size] = Int32GetDatum(l3cache_size.intVal);
			values[Anum_number_of_cores] = Int32GetDatum(no_of_cores.intVal);
			values[Anum_number_of_enabled_cores] = Int32GetDatum(no_of_enabled_cores.intVal);
			values[Anum_number_of_logical_processor] = Int32GetDatum(no_of_logical_processor.intVal);
			values[Anum_status_info] = Int32GetDatum(status_info.intVal);
			values[Anum_thread_count] = Int32GetDatum(thread_count.intVal);
			values[Anum_last_error_code] = Int32GetDatum(last_error_code.intVal);

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