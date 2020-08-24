/*------------------------------------------------------------------------
 * system_stats_utils.c
 *              Defined required utility functions to fetch
 *              system statistics information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */
#include "postgres.h"
#include "system_stats.h"

#include <windows.h>
#include <wbemidl.h>

#define THREAD_WAITING_STATE  5
#define THREAD_WAIT_REASON    5

// Initialize COM interface pointers
IWbemLocator         *locator = NULL;
IWbemServices        *services = NULL;

int is_process_running(int pid);

/* Check if all the threads of the process is running or not */
int is_process_running(int pid)
{
	// result code from COM calls from program
	HRESULT hres = 0;
	int ret_val = 0;
	char ptr[512] = { 0 };
	wchar_t  w_query[512];
	size_t outSize;
	char pid_str[20] = { 0 };
	sprintf(pid_str, "%d", pid);
	sprintf(ptr, "SELECT ThreadState, ThreadWaitReason FROM Win32_Thread WHERE ProcessHandle = %s", pid_str);
	mbstowcs_s(&outSize, w_query, sizeof(ptr), ptr, sizeof(ptr));

	IEnumWbemClassObject *results = NULL;
	BSTR query = SysAllocString(w_query);

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
			int wait_state = 0;
			int wait_reason = 0;

			hres = result->lpVtbl->Get(result, L"ThreadState", 0, &query_result, 0, 0);
			if (FAILED(hres))
			{
				elog(DEBUG1, "Failed to get ThreadState");
				ret_val = 0;
			}
			else
			{
				wait_state = query_result.intVal;
				hres = result->lpVtbl->Get(result, L"ThreadWaitReason", 0, &query_result, 0, 0);
				if (FAILED(hres))
				{
					elog(DEBUG1, "Failed to get ThreadWaitReason");
					ret_val = 0;
				}
				else
				{
					wait_reason = query_result.intVal;
					if (wait_state != THREAD_WAITING_STATE ||
						wait_reason != THREAD_WAIT_REASON)
					{
						ret_val = 1;
						break;
					}
				}
			}

			/* release the current result object */
			result->lpVtbl->Release(result);
		}

		/* release results set */
		results->lpVtbl->Release(results);
	}
	else
		ereport(DEBUG1, (errmsg("[is_process_running]: Failed to get query result")));

	SysFreeString(query);

	return ret_val;
}

void initialize_wmi_connection()
{
	// result code from COM calls from program
	HRESULT hres = 0;

	BSTR resource = SysAllocString(L"ROOT\\CIMV2");

	// initialize COM
	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[initialize_wmi_connection]: Failed to initialize COM library")));
		return;
	}

	hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[initialize_wmi_connection]: Failed to initialize the security")));
		CoUninitialize();
		SysFreeString(resource);
		return;
	}

	/* Obtain the initial locator to Windows Management on a particular host computer */
	hres = CoCreateInstance(&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (LPVOID *)&locator);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[initialize_wmi_connection]: Failed to create IWbemLocator object")));
		CoUninitialize();
		SysFreeString(resource);
		return;
	}

	/* Connect to the root\cimv2 namespace with the current user and obtain pointer services to make IWbemServices calls */
	hres = locator->lpVtbl->ConnectServer(locator, resource, NULL, NULL, NULL, 0, NULL, NULL, &services);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[initialize_wmi_connection]: Failed to create IWbemLocator object")));
		locator->lpVtbl->Release(locator);
		CoUninitialize();
		SysFreeString(resource);
		return;
	}

	ereport(DEBUG1, (errmsg("[initialize_wmi_connection]: Successfully initialized the service")));

	SysFreeString(resource);

	execute_init_query();
}

void uninitialize_wmi_connection()
{
	services->lpVtbl->Release(services);
	locator->lpVtbl->Release(locator);
	CoUninitialize();
}

/* Execute the query initially which took some time during execution
   so when user fire query, results will be given instantly */
void execute_init_query()
{
	IEnumWbemClassObject *results = NULL;

	// result code from COM calls from program
	HRESULT hres = 0;
	BSTR language = SysAllocString(L"WQL");

	if (services != NULL)
	{
		BSTR query = SysAllocString(L"SELECT * FROM Win32_PerfFormattedData_PerfProc_Process");

		// issue a WMI query
		hres = services->lpVtbl->ExecQuery(services, language, query, WBEM_FLAG_BIDIRECTIONAL, NULL, &results);
		if (FAILED(hres))
			ereport(DEBUG1, (errmsg("Failed to execute WQL query")));

		/* Ignore the result as we are not intersted in */
		if (results != NULL)
			results->lpVtbl->Release(results);

		SysFreeString(query);
	}

	SysFreeString(language);
}

IEnumWbemClassObject* execute_query(BSTR query)
{
	IEnumWbemClassObject *results = NULL;

	// result code from COM calls from program
	HRESULT hres = 0;
	BSTR language = SysAllocString(L"WQL");

	if (services != NULL)
	{
		// issue a WMI query
		hres = services->lpVtbl->ExecQuery(services, language, query, WBEM_FLAG_BIDIRECTIONAL, NULL, &results);
		if (FAILED(hres))
			ereport(DEBUG1, (errmsg("Failed to execute WQL query")));
	}
	else
	{
		/* Try to initialize it again and try one more time to execuet the query.
		 * It may possible that during first initliaze, resoufce may busy so try again */
		initialize_wmi_connection();
		if (services != NULL)
		{
			// issue a WMI query
			hres = services->lpVtbl->ExecQuery(services, language, query, WBEM_FLAG_BIDIRECTIONAL, NULL, &results);
			if (FAILED(hres))
				ereport(DEBUG1, (errmsg("Failed to execute WQL query")));
		}
	}

	SysFreeString(language);

	return results;
}
