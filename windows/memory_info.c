/*------------------------------------------------------------------------
 * memory_info.c
 *              System memory information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */
#include "postgres.h"
#include "stats.h"

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

void ReadMemoryInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum      values[Natts_memory_info];
	bool       nulls[Natts_memory_info];
	uint64     total_physical_memory = 0;
	uint64     avail_physical_memory = 0;
	uint64     memory_load_percentage = 0;
	uint64     total_page_file = 0;
	uint64     avail_page_file = 0;
	uint64     total_virtual_memory = 0;
	uint64     avail_virtual_memory = 0;
	uint64     avail_ext_virtual_memory = 0;
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
	memory_load_percentage = (uint64)statex.dwMemoryLoad;
	total_page_file = (uint64)statex.ullTotalPageFile;
	avail_page_file = (uint64)statex.ullAvailPageFile;
	total_virtual_memory = (uint64)statex.ullTotalVirtual;
	avail_virtual_memory = (uint64)statex.ullAvailVirtual;
	avail_ext_virtual_memory = (uint64)statex.ullAvailExtendedVirtual;	

	values[Anum_total_physical_memory]    = Int64GetDatumFast(total_physical_memory);
	values[Anum_avail_physical_memory]    = Int64GetDatumFast(avail_physical_memory);
	values[Anum_memory_load_percentage]   = Int64GetDatumFast(memory_load_percentage);
	values[Anum_total_page_file]          = Int64GetDatumFast(total_page_file);
	values[Anum_avail_page_file]          = Int64GetDatumFast(avail_page_file);
	values[Anum_total_virtual_memory]     = Int64GetDatumFast(total_virtual_memory);
	values[Anum_avail_virtual_memory]     = Int64GetDatumFast(avail_virtual_memory);
	values[Anum_avail_ext_virtual_memory] = Int64GetDatumFast(avail_ext_virtual_memory);

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);	
}