/*------------------------------------------------------------------------
 * io_analysis.c
 *              System IO analysis information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "stats.h"

#include <windows.h>
#include <wbemidl.h>

/* Function used to get IO statistics of block devices */
void ReadIOAnalysisInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum            values[Natts_disk_io_info];
	bool             nulls[Natts_disk_io_info];

	memset(nulls, 0, sizeof(nulls));

	// result code from COM calls from program
	HRESULT hres = 0;
	IEnumWbemClassObject *results = NULL;
	BSTR query = SysAllocString(L"SELECT * FROM Win32_PerfRawData_PerfDisk_PhysicalDisk");

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
				nulls[Anum_disk_io_caption] = true;
			else
			{
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_disk_io_caption] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_disk_io_caption] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"Description", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_io_description] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_disk_io_description] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_disk_io_description] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"Name", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_io_name] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_disk_io_name] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					values[Anum_disk_io_name] = CStringGetTextDatum(dst);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"AvgDiskBytesPerRead", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_avg_disk_bytes_per_read] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_avg_disk_bytes_per_read] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_avg_disk_bytes_per_read] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"AvgDiskBytesPerTransfer", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_avg_disk_bytes_per_transfer] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_avg_disk_bytes_per_transfer] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_avg_disk_bytes_per_transfer] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"AvgDiskBytesPerWrite", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_avg_disk_bytes_per_write] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_avg_disk_bytes_per_write] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_avg_disk_bytes_per_write] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"AvgDiskQueueLength", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_avg_disk_queue_length] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_avg_disk_queue_length] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_avg_disk_queue_length] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"AvgDiskReadQueueLength", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_avg_disk_read_queue_length] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_avg_disk_read_queue_length] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_avg_disk_read_queue_length] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"AvgDiskSecPerRead", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_avg_disk_sec_per_read] = true;
			else
				values[Anum_avg_disk_sec_per_read] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"AvgDiskSecPerTransfer", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_avg_disk_sec_per_transfer] = true;
			else
				values[Anum_avg_disk_sec_per_transfer] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"AvgDiskSecPerWrite", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_avg_disk_sec_per_write] = true;
			else
				values[Anum_avg_disk_sec_per_write] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"AvgDiskWriteQueueLength", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_avg_disk_write_queue_length] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_avg_disk_write_queue_length] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_avg_disk_write_queue_length] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"CurrentDiskQueueLength", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_current_disk_queue_length] = true;
			else
				values[Anum_current_disk_queue_length] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"DiskBytesPerSec", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_bytes_per_sec] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_disk_bytes_per_sec] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_disk_bytes_per_sec] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"DiskReadBytesPerSec", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_read_bytes_per_sec] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_disk_read_bytes_per_sec] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_disk_read_bytes_per_sec] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"DiskReadsPerSec", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_reads_per_sec] = true;
			else
				values[Anum_disk_reads_per_sec] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"DiskTransfersPerSec", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_transfers_per_sec] = true;
			else
				values[Anum_disk_transfers_per_sec] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"DiskWriteBytesPerSec", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_write_bytes_per_sec] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_disk_write_bytes_per_sec] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_disk_write_bytes_per_sec] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"DiskWritesPerSec", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_writes_per_sec] = true;
			else
				values[Anum_disk_writes_per_sec] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"Frequency_PerfTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_freq_perf_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_disk_freq_perf_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_disk_freq_perf_time] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"Frequency_Sys100NS", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_freq_sys_100_ns] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_disk_freq_sys_100_ns] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_disk_freq_sys_100_ns] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"PercentDiskReadTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_percent_disk_read_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_percent_disk_read_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_percent_disk_read_time] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"PercentDiskTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_percent_disk_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_percent_disk_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_percent_disk_time] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"PercentDiskWriteTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_percent_disk_write_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_percent_disk_write_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_percent_disk_write_time] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"PercentIdleTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_percent_disk_idle_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_percent_disk_idle_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_percent_disk_idle_time] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"SplitIOPerSec", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_split_io_per_sec] = true;
			else
				values[Anum_disk_split_io_per_sec] = Int32GetDatum(query_result.intVal);

			hres = result->lpVtbl->Get(result, L"Timestamp_Object", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_timestamp_obj] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_disk_timestamp_obj] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_disk_timestamp_obj] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"Timestamp_PerfTime", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_timestamp_perf_time] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_disk_timestamp_perf_time] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_disk_timestamp_perf_time] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			hres = result->lpVtbl->Get(result, L"Timestamp_Sys100NS", 0, &query_result, 0, 0);
			if (FAILED(hres))
				nulls[Anum_disk_timestamp_sys_100_ns] = true;
			else
			{
				wstr_length = 0;
				charsConverted = 0;
				wstr_length = SysStringLen(query_result.bstrVal);
				if (wstr_length == 0)
					nulls[Anum_disk_timestamp_sys_100_ns] = true;
				else
				{
					dst = (char *)malloc(wstr_length + 10);
					memset(dst, 0x00, (wstr_length + 10));
					wcstombs_s(&charsConverted, dst, wstr_length + 10, query_result.bstrVal, wstr_length);
					long long val = strtoll(dst, NULL, 10);
					values[Anum_disk_timestamp_sys_100_ns] = Int64GetDatumFast(val);
					free(dst);
				}
			}

			tuplestore_putvalues(tupstore, tupdesc, values, nulls);

			/* release the current result object */
			result->lpVtbl->Release(result);
		}
	}
	else
		ereport(DEBUG1, (errmsg("[ReadIOAnalysisInformation]: Failed to get query result")));

	/* release WMI COM interfaces */
	results->lpVtbl->Release(results);
	SysFreeString(query);
}
