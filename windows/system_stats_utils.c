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
#include "stats.h"

#include <windows.h>
#include <wbemidl.h>

#define THREAD_WAITING_STATE  5
#define THREAD_WAIT_REASON    5

int is_process_running(IWbemServices *services, int pid);

/* Check if all the threads of the process is running or not */
int is_process_running(IWbemServices *services, int pid)
{
	// result code from COM calls from program
	HRESULT hres = 0;
	int ret_val = 0;
	char ptr[512] = { 0 };
	wchar_t  w_query[512];
	size_t outSize;
	char pid_str[20] = { 0 };
	sprintf(pid_str, "%d", pid);
	sprintf(ptr, "SELECT * FROM Win32_Thread WHERE ProcessHandle = %s", pid_str);
	mbstowcs_s(&outSize, w_query, sizeof(ptr), ptr, sizeof(ptr));

	IEnumWbemClassObject *results = NULL;
	BSTR language = SysAllocString(L"WQL");
	BSTR query = SysAllocString(w_query);

	// issue a WMI query
	hres = services->lpVtbl->ExecQuery(services, language, query, WBEM_FLAG_BIDIRECTIONAL, NULL, &results);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[GetThreadStateOfProcess]: Failed to execute WQL query")));
		return ret_val;
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
	}

	/* release WMI COM interfaces */
	results->lpVtbl->Release(results);

	SysFreeString(query);
	SysFreeString(language);

	return ret_val;
}