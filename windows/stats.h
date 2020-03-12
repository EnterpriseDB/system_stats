/*------------------------------------------------------------------------
 * stats.h
 *              Defined macros and function prototypes for system
 *              statistics information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */
#ifndef STATS_H
#define STATS_H

#include "access/tupdesc.h"
#include "utils/tuplestore.h"
#include "utils/builtins.h"

/* Macros for Memory information */
#define Natts_memory_info                  8
#define Anum_total_physical_memory         0
#define Anum_avail_physical_memory         1
#define Anum_memory_load_percentage        2
#define Anum_total_page_file               3
#define Anum_avail_page_file               4
#define Anum_total_virtual_memory          5
#define Anum_avail_virtual_memory          6
#define Anum_avail_ext_virtual_memory      7

/* Macros for OS information */
#define Natts_os_info                         11
#define Anum_os_name                          0
#define Anum_os_version                       1
#define Anum_os_build_version                 2
#define Anum_os_servicepack_major_version     3
#define Anum_os_servicepack_minor_version     4
#define Anum_host_name                        5
#define Anum_number_of_users                  6
#define Anum_number_of_licensed_users         7
#define Anum_os_architecture                  8
#define Anum_os_install_time                  9
#define Anum_os_boot_time                     10

/* Macros for CPU information */
#define Natts_cpu_info                       19
#define Anum_device_id                       0
#define Anum_description                     1
#define Anum_manufacturer                    2
#define Anum_name                            3
#define Anum_processor_type                  4
#define Anum_architecture                    5
#define Anum_max_clock_speed                 6
#define Anum_current_clock_speed             7
#define Anum_address_width                   8
#define Anum_cpu_status                      9
#define Anum_l2cache_size                    10
#define Anum_l3cache_size                    11
#define Anum_number_of_cores                 12
#define Anum_number_of_enabled_cores         13
#define Anum_number_of_logical_processor     14
#define Anum_status                          15
#define Anum_status_info                     16
#define Anum_thread_count                    17
#define Anum_last_error_code                 18

#endif // STATS_H