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
	IEnumWbemClassObject *results = NULL;
	BSTR query = SysAllocString(L"SELECT * FROM Win32_Process");

	// issue a WMI query
	results = execute_query(query);

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

				if (is_process_running(pid))
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

		/*  NULL the platform independent value */
		nulls[Anum_no_of_sleeping_processes] = true;
		nulls[Anum_no_of_zombie_processes] = true;

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);
	}
	else
		ereport(DEBUG1, (errmsg("[ReadProcessInformations]: Failed to get query result")));

	/* release WMI COM interfaces */
	results->lpVtbl->Release(results);
	SysFreeString(query);
}
