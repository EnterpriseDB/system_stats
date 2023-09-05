/*------------------------------------------------------------------------
 * io_analysis.c
 *              System IO analysis information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"

#include <windows.h>
#include <wbemidl.h>
#include <winioctl.h>

#define MAX_DRIVE_COUNT  32
#define MAX_DEVICE_PATH  1024

/* Function used to get IO statistics of block devices */
void ReadIOAnalysisInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum            values[Natts_io_analysis_info];
	bool             nulls[Natts_io_analysis_info];
	DISK_PERFORMANCE diskPerformance;
	HANDLE           hDevice = NULL;
	char             szDevice[MAX_DEVICE_PATH];
	char             szDeviceDisplay[MAX_DEVICE_PATH];
	int              deviceId;
	int              icount;
	DWORD            ioctrlSize;
	DWORD            dwSize;
	BOOL             ret;

	memset(nulls, 0, sizeof(nulls));
	memset(szDevice, 0, MAX_DEVICE_PATH);
	memset(szDeviceDisplay, 0, MAX_DEVICE_PATH);

	for (deviceId = 0; deviceId <= MAX_DRIVE_COUNT; ++deviceId)
	{
		sprintf_s(szDevice, MAX_DEVICE_PATH, "\\\\.\\PhysicalDrive%d", deviceId);

		hDevice = CreateFile(szDevice, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL);

		if (hDevice == INVALID_HANDLE_VALUE)
			continue;

		icount = 0;
		ioctrlSize = sizeof(diskPerformance);
		while (1)
		{
			icount += 1;
			ret = DeviceIoControl(
				hDevice, IOCTL_DISK_PERFORMANCE, NULL, 0, &diskPerformance,
				ioctrlSize, &dwSize, NULL);

			if (ret != 0)
				break;

			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				if (icount <= 1024)
				{
					ioctrlSize *= 2;
					continue;
				}
			}
			else if (GetLastError() == ERROR_INVALID_FUNCTION)
			{
				printf("DeviceIoControl->ERROR_INVALID_FUNCTION; ignore PhysicalDrive%d", deviceId);
				if (hDevice != NULL)
					CloseHandle(hDevice);
				return;
			}
			else if (GetLastError() == ERROR_NOT_SUPPORTED)
			{
				printf("DeviceIoControl -> ERROR_NOT_SUPPORTED; ignore PhysicalDrive%d", deviceId);
				if (hDevice != NULL)
					CloseHandle(hDevice);
				return;
			}

			if (hDevice != NULL)
				CloseHandle(hDevice);

			return;
		}

		sprintf_s(szDeviceDisplay, MAX_DEVICE_PATH, "PhysicalDrive%i", deviceId);

		values[Anum_device_name] = CStringGetTextDatum(szDeviceDisplay);
		values[Anum_total_read] = UInt64GetDatum((uint64)diskPerformance.ReadCount);
		values[Anum_total_write] = UInt64GetDatum((uint64)diskPerformance.WriteCount);
		values[Anum_read_bytes] = UInt64GetDatum((uint64)(diskPerformance.BytesRead.QuadPart));
		values[Anum_write_bytes] = UInt64GetDatum((uint64)(diskPerformance.BytesWritten.QuadPart));
		values[Anum_read_time_ms] = UInt64GetDatum((uint64)(diskPerformance.ReadTime.QuadPart) / 10000000);
		values[Anum_write_time_ms] = UInt64GetDatum((uint64)(diskPerformance.WriteTime.QuadPart) / 10000000);

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);

		if (hDevice != NULL)
			CloseHandle(hDevice);
	}
}
