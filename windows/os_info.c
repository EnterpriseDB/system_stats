/*------------------------------------------------------------------------
 * os_info.c
 *              Operating system information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"

#include <windows.h>
#include <wbemidl.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

void ReadOSInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum            values[Natts_os_info];
	bool             nulls[Natts_os_info];
	int              handle_count = 0;
	int              process_count = 0;
	int              thread_count = 0;

	memset(nulls, 0, sizeof(nulls));

	PERFORMANCE_INFORMATION per_statex;
	per_statex.cb = sizeof(per_statex);
	if (GetPerformanceInfo(&per_statex, per_statex.cb) == 0)
	{
		LPVOID lpMsgBuf;
		DWORD dw = GetLastError();
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
		ereport(DEBUG1, (errmsg("Error while getting memory peformance information: %s", (char *)lpMsgBuf)));
		LocalFree(lpMsgBuf);
	}
	else
	{
		handle_count = (int)(per_statex.HandleCount);
		process_count = (int)(per_statex.ProcessCount);
		thread_count = (int)(per_statex.ThreadCount);
	}

	// result code from COM calls from program
	HRESULT hres = 0;
	IEnumWbemClassObject *results = NULL;
	BSTR query = SysAllocString(L"SELECT * FROM Win32_Operatingsystem");

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
			hres = result->lpVtbl->Get(result, L"Caption", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_os_name] = true;
			else
			{
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_os_name] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_os_name] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"Version", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_os_version] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_os_version] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_os_version] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"CSName", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_host_name] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_host_name] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_host_name] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"OSArchitecture", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_os_architecture] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_os_architecture] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_os_architecture] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"LastBootUpTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_os_boot_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_os_boot_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_os_boot_time] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			values[Anum_os_handle_count] = handle_count;
			values[Anum_os_process_count] = process_count;
			values[Anum_os_thread_count] = thread_count;

			/* set the NULL value for column which is not required by this platform */
			nulls[Anum_domain_name] = true;
			nulls[Anum_os_up_since_seconds] = true;

			tuplestore_putvalues(tupstore, tupdesc, values, nulls);

			/* release the current result object */
			result->lpVtbl->Release(result);
		}

		/* release results set */
		results->lpVtbl->Release(results);
	}
	else
		ereport(DEBUG1, (errmsg("[ReadOSInformations]: Failed to get query result")));

	SysFreeString(query);
}
