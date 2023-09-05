/*------------------------------------------------------------------------
 * memory_info.c
 *              System memory information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */
#include "postgres.h"
#include "system_stats.h"

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

void ReadMemoryInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum      values[Natts_memory_info];
	bool       nulls[Natts_memory_info];
	uint64     total_physical_memory = 0;
	uint64     avail_physical_memory = 0;
	uint64     memory_load_percentage = 0;
	uint64     total_page_file = 0;
	uint64     avail_page_file = 0;
	uint64     total_system_cache = 0;
	uint64     kernel_total = 0;
	uint64     kernel_paged = 0;
	uint64     kernel_non_paged = 0;
	MEMORYSTATUSEX statex;

	memset(nulls, 0, sizeof(nulls));

	statex.dwLength = sizeof(statex);

	if (GlobalMemoryStatusEx(&statex) == 0)
	{
		LPVOID lpMsgBuf;
		DWORD dw = GetLastError();
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
		ereport(DEBUG1,  (errmsg("Error while getting memory information: %s", (char *)lpMsgBuf)));
		LocalFree(lpMsgBuf);
		return;
	}

	total_physical_memory = (uint64)statex.ullTotalPhys;
	avail_physical_memory = (uint64)statex.ullAvailPhys;
	total_page_file = (uint64)statex.ullTotalPageFile;
	avail_page_file = (uint64)statex.ullAvailPageFile;

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
		uint64 page_size = (uint64)per_statex.PageSize;
		total_system_cache = ((uint64)(per_statex.SystemCache)) * page_size;
		kernel_total = ((uint64)(per_statex.KernelTotal)) * page_size;
		kernel_paged = ((uint64)(per_statex.KernelPaged)) * page_size;
		kernel_non_paged = ((uint64)(per_statex.KernelNonpaged)) * page_size;
	}

	values[Anum_total_memory]           = UInt64GetDatum(total_physical_memory);
	values[Anum_used_memory]            = UInt64GetDatum((total_physical_memory - avail_physical_memory));
	values[Anum_free_memory]            = UInt64GetDatum(avail_physical_memory);
	values[Anum_total_cache_memory]     = UInt64GetDatum(total_system_cache);
	values[Anum_kernel_total_memory]    = UInt64GetDatum(kernel_total);
	values[Anum_kernel_paged_memory]    = UInt64GetDatum(kernel_paged);
	values[Anum_kernel_nonpaged_memory] = UInt64GetDatum(kernel_non_paged);
	values[Anum_total_page_file]        = UInt64GetDatum(total_page_file);
	values[Anum_avail_page_file]        = UInt64GetDatum(avail_page_file);

	/* NULL the value which is not required for this platform */
	nulls[Anum_swap_total_memory] = true;
	nulls[Anum_swap_used_memory] = true;
	nulls[Anum_swap_free_memory] = true;

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);
}
