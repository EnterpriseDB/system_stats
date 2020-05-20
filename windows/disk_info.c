/*------------------------------------------------------------------------
 * disk_info.c
 *              System disk information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"

#include <windows.h>
#include <wbemidl.h>

void ReadDiskInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum            values[Natts_disk_info];
	bool             nulls[Natts_disk_info];

	memset(nulls, 0, sizeof(nulls));

	// result code from COM calls from program
	HRESULT hres = 0;
	IEnumWbemClassObject *results = NULL;
	BSTR query = SysAllocString(L"SELECT * FROM Win32_Volume");

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
			uint64  total_space, free_space, used_space;

			int     wstr_length = 0;
			char    *dst = NULL;
			size_t  charsConverted = 0;

			hres = result->lpVtbl->Get(result, L"DriveLetter", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_drive_letter] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_disk_drive_letter] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_disk_drive_letter] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"DriveType", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_drive_type] = true;
			else
				values[Anum_disk_drive_type] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"FileSystem", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_file_system] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_disk_file_system] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_disk_file_system] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"FreeSpace", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_free_space] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_disk_free_space] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					free_space = strtoll(dst, NULL, 10);
					values[Anum_disk_free_space] = Int64GetDatumFast(free_space);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"Capacity", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_total_space] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_disk_total_space] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					total_space = strtoll(dst, NULL, 10);
					values[Anum_disk_total_space] = Int64GetDatumFast(total_space);
					free(dst);
				}
			}

			used_space = (total_space - free_space);
			values[Anum_disk_used_space] = Int64GetDatumFast(used_space);

			nulls[Anum_disk_mount_point] = true;
			nulls[Anum_disk_file_system_type] = true;
			nulls[Anum_disk_total_inodes] = true;
			nulls[Anum_disk_used_inodes] = true;
			nulls[Anum_disk_free_inodes] = true;

			tuplestore_putvalues(tupstore, tupdesc, values, nulls);

			/* release the current result object */
			result->lpVtbl->Release(result);
		}
	}
	else
		ereport(DEBUG1, (errmsg("[ReadDiskInformation]: Failed to get query result")));

	/* release WMI COM interfaces */
	results->lpVtbl->Release(results);
	SysFreeString(query);
}
