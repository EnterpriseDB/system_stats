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

/* Here Size is defined in both postgres and Mac Foundation class
   so defined it here to avoid compilation error */
#define Size size_mac
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOBlockStorageDriver.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/IOBSD.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#undef Size

/* Function used to get IO statistics of block devices */
void ReadIOAnalysisInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum                values[Natts_io_analysis_info];
	bool                 nulls[Natts_io_analysis_info];
    CFDictionaryRef      disk_parent_dict;
    CFDictionaryRef      disk_prop_dict;
    CFDictionaryRef      disk_stats_dict;
    io_registry_entry_t  parent_reg_ent;
    io_registry_entry_t  disk_reg_ent;
    io_iterator_t        disk_list_iter;

	memset(nulls, 0, sizeof(nulls));

    // Get list of disks
    if (IOServiceGetMatchingServices(kIOMasterPortDefault,
            IOServiceMatching(kIOMediaClass),
            &disk_list_iter) != kIOReturnSuccess)
    {
        ereport(DEBUG1, (errmsg("Failed to get the list of the disks")));
        return;
    }

    // Iterate over disks
    while ((disk_reg_ent = IOIteratorNext(disk_list_iter)) != 0)
    {
        disk_parent_dict = NULL;
        disk_prop_dict = NULL;
        disk_stats_dict = NULL;

        if (IORegistryEntryGetParentEntry(disk_reg_ent,
                kIOServicePlane, &parent_reg_ent) != kIOReturnSuccess)
        {
            ereport(DEBUG1, (errmsg("Failed to get the parent of the disk")));
            IOObjectRelease(disk_reg_ent);
            IOObjectRelease (disk_list_iter);
            return;
        }

        if (IOObjectConformsTo(parent_reg_ent, "IOBlockStorageDriver"))
        {
            if (IORegistryEntryCreateCFProperties(
                    disk_reg_ent,
                    (CFMutableDictionaryRef *) &disk_parent_dict,
                    kCFAllocatorDefault,
                    kNilOptions
                ) != kIOReturnSuccess)
            {
                ereport(DEBUG1, (errmsg("Failed to get the parent's disk properties")));
                IOObjectRelease(disk_reg_ent);
                IOObjectRelease(parent_reg_ent);
                IOObjectRelease (disk_list_iter);
                return;
            }

            if (IORegistryEntryCreateCFProperties(
                    parent_reg_ent,
                    (CFMutableDictionaryRef *) &disk_prop_dict,
                    kCFAllocatorDefault,
                    kNilOptions) != kIOReturnSuccess)
            {
                ereport(DEBUG1, (errmsg("Failed to get the disk properties")));
                CFRelease(disk_parent_dict);
                IOObjectRelease(disk_reg_ent);
                IOObjectRelease(parent_reg_ent);
                IOObjectRelease (disk_list_iter);
                return;
            }

            const int kMaxDiskNameSize = 64;
            CFStringRef disk_name_ref = (CFStringRef)CFDictionaryGetValue(
                    disk_parent_dict, CFSTR(kIOBSDNameKey));
            char device_name[kMaxDiskNameSize];

            CFStringGetCString(disk_name_ref,
                                device_name,
                                kMaxDiskNameSize,
                                CFStringGetSystemEncoding());

            disk_stats_dict = (CFDictionaryRef)CFDictionaryGetValue(
                            disk_prop_dict, CFSTR(kIOBlockStorageDriverStatisticsKey));

            if (disk_stats_dict == NULL)
            {
                ereport(DEBUG1, (errmsg("Failed to get the disk stats")));
                CFRelease(disk_parent_dict);
                IOObjectRelease(parent_reg_ent);
                CFRelease(disk_prop_dict);
                IOObjectRelease(disk_reg_ent);
                IOObjectRelease (disk_list_iter);
                return;
            }

            CFNumberRef number;
            int64_t reads = 0;
            int64_t writes = 0;
            int64_t read_bytes = 0;
            int64_t write_bytes = 0;
            int64_t read_time_ns = 0;
            int64_t write_time_ns = 0;

            // Get disk reads/writes
            if ((number = (CFNumberRef)CFDictionaryGetValue(
                    disk_stats_dict,
                    CFSTR(kIOBlockStorageDriverStatisticsReadsKey))))
            {
                CFNumberGetValue(number, kCFNumberSInt64Type, &reads);
            }
            if ((number = (CFNumberRef)CFDictionaryGetValue(
                    disk_stats_dict,
                    CFSTR(kIOBlockStorageDriverStatisticsWritesKey))))
            {
                CFNumberGetValue(number, kCFNumberSInt64Type, &writes);
            }

            // Get disk bytes read/written
            if ((number = (CFNumberRef)CFDictionaryGetValue(
                    disk_stats_dict,
                    CFSTR(kIOBlockStorageDriverStatisticsBytesReadKey))))
            {
                CFNumberGetValue(number, kCFNumberSInt64Type, &read_bytes);
            }
            if ((number = (CFNumberRef)CFDictionaryGetValue(
                    disk_stats_dict,
                    CFSTR(kIOBlockStorageDriverStatisticsBytesWrittenKey))))
            {
                CFNumberGetValue(number, kCFNumberSInt64Type, &write_bytes);
            }

            // Get disk time spent reading/writing (nanoseconds)
            if ((number = (CFNumberRef)CFDictionaryGetValue(
                    disk_stats_dict,
                    CFSTR(kIOBlockStorageDriverStatisticsTotalReadTimeKey))))
            {
                CFNumberGetValue(number, kCFNumberSInt64Type, &read_time_ns);
            }
            if ((number = (CFNumberRef)CFDictionaryGetValue(
                    disk_stats_dict,
                    CFSTR(kIOBlockStorageDriverStatisticsTotalWriteTimeKey))))
            {
                CFNumberGetValue(number, kCFNumberSInt64Type, &write_time_ns);
            }

            /* convert nano second to milli second */
            read_time_ns = (int64_t)round(read_time_ns/1000000);
            write_time_ns = (int64_t)round(write_time_ns/1000000);

		    values[Anum_device_name] = CStringGetTextDatum(device_name);
		    values[Anum_total_read] = Int64GetDatumFast(reads);
		    values[Anum_total_write] = Int64GetDatumFast(writes);
		    values[Anum_read_bytes] = Int64GetDatumFast(read_bytes);
		    values[Anum_write_bytes] = Int64GetDatumFast(write_bytes);
		    values[Anum_read_time_ms] = Int64GetDatumFast(read_time_ns);
		    values[Anum_write_time_ms] = Int64GetDatumFast(write_time_ns);

		    tuplestore_putvalues(tupstore, tupdesc, values, nulls);

            CFRelease(disk_parent_dict);
            IOObjectRelease(parent_reg_ent);
            CFRelease(disk_prop_dict);
            IOObjectRelease(disk_reg_ent);
        }

    }

    IOObjectRelease (disk_list_iter);
}
