/*------------------------------------------------------------------------
 * os_info.c
 *              Operating system information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "stats.h"

#include <windows.h>
#include <wbemidl.h>

void ReadOSInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum            values[Natts_os_info];
	bool             nulls[Natts_os_info];

	memset(nulls, 0, sizeof(nulls));

	// result code from COM calls from program
	HRESULT hres = 0;

	// Initialize COM interface pointers
	IWbemLocator         *locator = NULL;
	IWbemServices        *services = NULL;
	IEnumWbemClassObject *results = NULL;

	BSTR resource = SysAllocString(L"ROOT\\CIMV2");
	BSTR language = SysAllocString(L"WQL");
	BSTR query = SysAllocString(L"SELECT * FROM Win32_Operatingsystem");

	// initialize COM
	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadOSInformations]: Failed to initialize COM library")));
		return;
	}

	hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadOSInformations]: Failed to initialize the security")));
		CoUninitialize();
		return;
	}

	/* Obtain the initial locator to Windows Management on a particular host computer */
	hres = CoCreateInstance(&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (LPVOID *)&locator);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadOSInformations]: Failed to create IWbemLocator object")));
		CoUninitialize();
		return;
	}

	/* Connect to the root\cimv2 namespace with the current user and obtain pointer services to make IWbemServices calls */
	hres = locator->lpVtbl->ConnectServer(locator, resource, NULL, NULL, NULL, 0, NULL, NULL, &services);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadOSInformations]: Failed to create IWbemLocator object")));
		locator->lpVtbl->Release(locator);
		CoUninitialize();
		return;
	}

	// issue a WMI query
	hres = services->lpVtbl->ExecQuery(services, language, query, WBEM_FLAG_BIDIRECTIONAL, NULL, &results);
	if (FAILED(hres))
	{
		ereport(DEBUG1, (errmsg("[ReadOSInformations]: Failed to execute WQL query")));
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

			hres = result->lpVtbl->Get(result, L"BuildNumber", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_os_build_version] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_os_build_version] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_os_build_version] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"ServicePackMajorVersion", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_os_servicepack_major_version] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_os_servicepack_major_version] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_os_servicepack_major_version] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"ServicePackMinorVersion", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_os_servicepack_minor_version] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_os_servicepack_minor_version] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_os_servicepack_minor_version] = CStringGetTextDatum(dst);
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

			hres = result->lpVtbl->Get(result, L"NumberOfUsers", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_number_of_users] = true;
			else
				values[Anum_number_of_users] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"NumberOfLicensedUsers", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_number_of_licensed_users] = true;
			else
				values[Anum_number_of_licensed_users] = Int32GetDatum(query_result.intVal);

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

			hres = result->lpVtbl->Get(result, L"InstallDate", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_os_install_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_os_install_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_os_install_time] = CStringGetTextDatum(dst);
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