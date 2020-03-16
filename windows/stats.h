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

/* Macros for disk information */
#define Natts_disk_info                       14
#define Anum_disk_device_id                   0
#define Anum_disk_caption                     1
#define Anum_disk_description                 2
#define Anum_disk_name                        3
#define Anum_disk_max_file_name_length        4
#define Anum_disk_block_size                  5
#define Anum_disk_no_of_blocks                6
#define Anum_drive_type                       7
#define Anum_file_system                      8
#define Anum_disk_free_space                  9
#define Anum_disk_total_space                 10
#define Anum_disk_used_space                  11
#define Anum_disk_drive_letter                12
#define Anum_disk_last_error_code             13

/* Macros for disk IO information */
#define Natts_disk_io_info                   29
#define Anum_disk_io_caption                 0
#define Anum_disk_io_name                    1
#define Anum_disk_io_description             2
#define Anum_avg_disk_bytes_per_read         3
#define Anum_avg_disk_bytes_per_transfer     4
#define Anum_avg_disk_bytes_per_write        5
#define Anum_avg_disk_queue_length           6
#define Anum_avg_disk_read_queue_length      7
#define Anum_avg_disk_sec_per_read           8
#define Anum_avg_disk_sec_per_transfer       9
#define Anum_avg_disk_sec_per_write          10
#define Anum_avg_disk_write_queue_length     11
#define Anum_current_disk_queue_length       12
#define Anum_disk_bytes_per_sec              13
#define Anum_disk_read_bytes_per_sec         14
#define Anum_disk_reads_per_sec              15
#define Anum_disk_transfers_per_sec          16
#define Anum_disk_write_bytes_per_sec        17
#define Anum_disk_writes_per_sec             18
#define Anum_disk_freq_perf_time             19
#define Anum_disk_freq_sys_100_ns            20
#define Anum_percent_disk_read_time          21
#define Anum_percent_disk_time               22
#define Anum_percent_disk_write_time         23
#define Anum_percent_disk_idle_time          24
#define Anum_disk_split_io_per_sec           25
#define Anum_disk_timestamp_obj              26
#define Anum_disk_timestamp_perf_time        27
#define Anum_disk_timestamp_sys_100_ns       28

/* Macros for CPU usage information */
#define Natts_cpu_usage_info                8
#define Anum_cpu_usage_name                 0
#define Anum_cpu_usage_caption              1
#define Anum_cpu_usage_description          2
#define Anum_cpu_percent_idle_time          3
#define Anum_cpu_percent_interrupt_time     4
#define Anum_cpu_percent_privileged_time    5
#define Anum_cpu_percent_processor_time     6
#define Anum_cpu_percent_user_time          7

#endif // STATS_H