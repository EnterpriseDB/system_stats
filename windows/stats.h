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

#include <windows.h>
#include <wbemidl.h>

void initialize_wmi_connection();
void uninitialize_wmi_connection();
void execute_init_query();
IEnumWbemClassObject* execute_query(BSTR query);

int is_process_running(int pid);

/* Macros for Memory information */
#define Natts_memory_info             12
#define Anum_total_memory             0
#define Anum_used_memory              1
#define Anum_free_memory              2
#define Anum_swap_total_memory        3
#define Anum_swap_used_memory         4
#define Anum_swap_free_memory         5
#define Anum_total_cache_memory       6
#define Anum_kernel_total_memory      7
#define Anum_kernel_paged_memory      8
#define Anum_kernel_nonpaged_memory   9
#define Anum_total_page_file          10
#define Anum_avail_page_file          11

/* Macros for OS information */
#define Natts_os_info                         12
#define Anum_os_name                          0
#define Anum_os_version                       1
#define Anum_host_name                        2
#define Anum_domain_name                      3
#define Anum_number_of_users                  4
#define Anum_number_of_licensed_users         5
#define Anum_os_handle_count                  6
#define Anum_os_process_count                 7
#define Anum_os_thread_count                  8
#define Anum_os_architecture                  9
#define Anum_os_boot_time                     10
#define Anum_os_up_since_seconds              11

/* Macros for CPU information */
#define Natts_cpu_info                       13
#define Anum_cpu_vendor                      0
#define Anum_cpu_description                 1
#define Anum_model_name                      2
#define Anum_processor_type                  3
#define Anum_logical_processor               4
#define Anum_physical_processor              5
#define Anum_no_of_cores                     6
#define Anum_architecture                    7
#define Anum_cpu_clock_speed                 8
#define Anum_l1dcache_size                   9
#define Anum_l1icache_size                   10
#define Anum_l2cache_size                    11
#define Anum_l3cache_size                    12

/* Macros for disk information */
#define Natts_disk_info                         11
#define Anum_disk_mount_point                   0
#define Anum_disk_file_system                   1
#define Anum_disk_drive_letter                  2
#define Anum_disk_drive_type                    3
#define Anum_disk_file_system_type              4
#define Anum_disk_total_space                   5
#define Anum_disk_used_space                    6
#define Anum_disk_free_space                    7
#define Anum_disk_total_inodes                  8
#define Anum_disk_used_inodes                   9
#define Anum_disk_free_inodes                   10

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

/* Macros for network information */
#define Natts_network_info          11
#define Anum_net_interface_name     0
#define Anum_net_ipv4_address       1
#define Anum_net_tx_bytes           2
#define Anum_net_tx_packets         3
#define Anum_net_tx_errors          4
#define Anum_net_tx_dropped         5
#define Anum_net_rx_bytes           6
#define Anum_net_rx_packets         7
#define Anum_net_rx_errors          8
#define Anum_net_rx_dropped         9
#define Anum_net_speed_mbps         10

/* Macros for cpu and memory information by process*/
#define Natts_process_info                   5
#define Anum_no_of_total_processes           0
#define Anum_no_of_running_processes         1
#define Anum_no_of_stopped_processes         2
#define Anum_no_of_sleeping_processes        3
#define Anum_no_of_zombie_processes          4

/* Macros for cpu and memory information by process*/
#define Natts_cpu_memory_info_by_process     6
#define Anum_process_pid                     0
#define Anum_process_name                    1
#define Anum_process_running_since           2
#define Anum_percent_cpu_usage               3
#define Anum_percent_memory_usage            4
#define Anum_process_memory_bytes            5

/* Macro for load average information */
#define Natts_load_avg_info                  4
#define Anum_load_avg_one_minute             0
#define Anum_load_avg_five_minutes           1
#define Anum_load_avg_ten_minutes            2
#define Anum_load_avg_fifteen_minutes        3

#endif // STATS_H