/*------------------------------------------------------------------------
 * process_info.c
 *              System process information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "stats.h"

#include <windows.h>
#include <wbemidl.h>

void ReadProcessInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum            values[Natts_process_info];
	bool             nulls[Natts_process_info];

	memset(nulls, 0, sizeof(nulls));

	// result code from COM calls from program
	HRESULT hres = 0;

	// Initialize COM interface pointers
	IWbemLocator         *locator = NULL;
	IWbemServices        *services = NULL;
	IEnumWbemClassObject *results = NULL;
	IEnumWbemClassObject *iresults = NULL;

	BSTR resource = SysAllocString(L"ROOT\\CIMV2");
	BSTR language = SysAllocString(L"WQL");
	BSTR query = SysAllocString(L"SELECT * FROM Win32_Process");

	// initialize COM
	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadProcessInformations]: Failed to initialize COM library")));
		return;
	}

	hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadProcessInformations]: Failed to initialize the security")));
		CoUninitialize();
		return;
	}

	/* Obtain the initial locator to Windows Management on a particular host computer */
	hres = CoCreateInstance(&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (LPVOID *)&locator);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadProcessInformations]: Failed to create IWbemLocator object")));
		CoUninitialize();
		return;
	}

	/* Connect to the root\cimv2 namespace with the current user and obtain pointer services to make IWbemServices calls */
	hres = locator->lpVtbl->ConnectServer(locator, resource, NULL, NULL, NULL, 0, NULL, NULL, &services);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadProcessInformations]: Failed to create IWbemLocator object")));
		locator->lpVtbl->Release(locator);
		CoUninitialize();
		return;
	}

	// issue a WMI query
	hres = services->lpVtbl->ExecQuery(services, language, query, WBEM_FLAG_BIDIRECTIONAL, NULL, &results);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadProcessInformations]: Failed to execute WQL query")));
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
		int no_of_total_processes = 0;
		int no_of_running_processes = 0;
		int no_of_stopped_processes = 0;

		// enumerate the retrieved objects
		while ((hres = results->lpVtbl->Next(results, WBEM_INFINITE, 1, &result, &returnedCount)) == S_OK)
		{
			VARIANT query_result;
			int     pid = 0;

			/* Get the value from query output */
			hres = result->lpVtbl->Get(result, L"ProcessId", 0, &query_result, 0, 0);
			if (FAILED(hres))
				continue;
			else
			{
				pid = query_result.intVal;

				no_of_total_processes++;

				if (is_process_running(services, pid))
					no_of_running_processes++;
				else
					no_of_stopped_processes++;
			}

			/* release the current result object */
			result->lpVtbl->Release(result);
		}

		values[Anum_no_of_total_processes] = Int32GetDatum(no_of_total_processes);
		values[Anum_no_of_running_processes] = Int32GetDatum(no_of_running_processes);
		values[Anum_no_of_stopped_processes] = Int32GetDatum(no_of_stopped_processes);

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);
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
