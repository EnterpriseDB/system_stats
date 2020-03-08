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

/* Macros for system disk information */
#define Natts_disk_info                10
#define IGNORE_MOUNT_POINTS_REGEX      "^/(dev|proc|sys|run|snap|var/lib/docker/.+)($|/)"
#define IGNORE_FILE_SYSTEM_TYPE_REGEX  "^(autofs|binfmt_misc|bpf|cgroup2?|configfs|debugfs|devpts|devtmpfs|fusectl|hugetlbfs|iso9660|mqueue|nsfs|overlay|proc|procfs|pstore|rpc_pipefs|securityfs|selinuxfs|squashfs|sysfs|tracefs)$"
#define Anum_file_system               0
#define Anum_file_system_type          1
#define Anum_mount_point               2
#define Anum_total_space               3
#define Anum_used_space                4
#define Anum_available_space           5
#define Anum_reserved_space            6
#define Anum_total_inodes              7
#define Anum_used_inodes               8
#define Anum_free_inodes               9

/* prototypes for system disk information functions */
bool ignoreFileSystemTypes(char *fs_mnt);
bool ignoreMountPoints(char *fs_mnt);

/* Macros for system IO Analysis */
#define Natts_io_analysis_info            7
#define Anum_device_name                  0
#define Anum_total_read                   1
#define Anum_total_write                  2
#define Anum_read_bytes                   3
#define Anum_write_bytes                  4
#define Anum_read_time_ms                 5
#define Anum_write_time_ms                6

/* Macros for system CPU information */
#define Natts_cpu_info            13
#define Anum_active_cpu           0
#define Anum_byte_order           1
#define Anum_cpu_family           2
#define Anum_cpu_type             3
#define Anum_logical_cpu          4
#define Anum_physical_cpu         5
#define Anum_cpu_freq_hz          6
#define Anum_cpu_model            7
#define Anum_cpu_machine          8
#define Anum_l1dcache_size        9
#define Anum_l1icache_size        10
#define Anum_l2cache_size         11
#define Anum_l3cache_size         12

/* Macros for Memory information */
#define Natts_memory_info       6
#define Anum_total_memory       0
#define Anum_used_memory        1
#define Anum_free_memory        2
#define Anum_swap_total         3
#define Anum_swap_used          4
#define Anum_swap_free          5

uint64_t ConvertToBytes(char *line_buf);

/* Macros for load average information */
#define Natts_load_avg_info                    3
#define Anum_cpu_io_load_avg_one_minute        0
#define Anum_cpu_io_load_avg_five_minutes      1
#define Anum_cpu_io_load_avg_fifteen_minutes   2

/* Macros for operating system information */
#define Natts_os_info                6
#define Anum_host_name               0
#define Anum_domain_name             1
#define Anum_os_name                 2
#define Anum_os_release_level        3
#define Anum_os_version_level        4
#define Anum_architecture            5

/* Macros for system CPU usage information */
#define Natts_cpu_usage_stats                            5
#define Anum_usermode_normal_process_cpu_percentage      0
#define Anum_usermode_niced_process_cpu_percentage       1
#define Anum_kernelmode_process_cpu_percentage           2
#define Anum_idle_mode_cpu_percentage                    3
#define Anum_total_percentage_cpu                        4

/* Macros for system processes information */
#define Natts_process_info                    5
#define Anum_total_processes                  0
#define Anum_running_processes                1
#define Anum_sleeping_processes               2
#define Anum_stopped_processes                3
#define Anum_zombie_processes                 4

/* Macros for network information */
#define Natts_network_info          11
#define Anum_net_interface_name     0
#define Anum_net_ipv4_address       1
#define Anum_net_speed_mbps         2
#define Anum_net_tx_bytes           3
#define Anum_net_tx_packets         4
#define Anum_net_tx_errors          5
#define Anum_net_tx_dropped         6
#define Anum_net_rx_bytes           7
#define Anum_net_rx_packets         8
#define Anum_net_rx_errors          9
#define Anum_net_rx_dropped         10

/* Macros for cpu and memory information by process*/
#define Natts_cpu_memory_info_by_process     4
#define Anum_process_pid                     0
#define Anum_process_command                 1
#define Anum_process_cpu_usage               2
#define Anum_process_memory_usage            3

#endif // STATS_H
