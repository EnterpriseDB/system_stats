/*------------------------------------------------------------------------
 * system_stats.h
 *              Defined macros and function prototypes for system
 *              statistics information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */
#ifndef SYSTEM_STATS_H
#define SYSTEM_STATS_H

#include "access/tupdesc.h"
#include "utils/tuplestore.h"
#include "utils/builtins.h"

/* read the the output of command in chunk of 1024 bytes */
#define READ_CHUNK_BYTES     1024
#define MIN_BUFFER_SIZE      512
#define MAX_BUFFER_SIZE      2048
#define IS_EMPTY_STR(X) ((1 / (sizeof(X[0]) == 1)) && !(X[0]))
#define PROC_FILE_SYSTEM_PATH    "/proc"

/* prototypes for common string manipulations and command execution functions */
bool stringIsNumber(char *str);
char* lefttrimStr(char *);
char* righttrimStr(char *);
char*  trimStr(char *);
char* runCommand(char *);

/* Macros for system disk information */
#define Natts_disk_info                10
#define FILE_SYSTEM_MOUNT_FILE_NAME    "/etc/mtab"
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
void ReadDiskInformation(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* Macros for system IO Analysis */
#define Natts_io_analysis_info            14
#define DISK_IO_STATS_FILE_NAME           "/proc/diskstats"
#define Anum_major_no                     0
#define Anum_minor_no                     1
#define Anum_device_name                  2
#define Anum_read_completed               3
#define Anum_read_merged                  4
#define Anum_sector_read                  5
#define Anum_time_spent_reading_ms        6
#define Anum_write_completed              7
#define Anum_write_merged                 8
#define Anum_sector_written               9
#define Anum_time_spent_writing_ms        10
#define Anum_io_in_progress               11
#define Anum_time_spent_io_ms             12
#define Anum_weighted_time_spent_io_ms    13

/* prototypes for system IO analysis functions */
void ReadIOAnalysisInformation(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* Macros for system CPU information */
#define Natts_cpu_info            7
#define CPU_INFO_FILE_NAME        "/proc/cpuinfo"
#define Anum_processor            0
#define Anum_vendor_id            1
#define Anum_cpu_family           2
#define Anum_model                3
#define Anum_model_name           4
#define Anum_cpu_mhz              5
#define Anum_cpu_cache_size       6

/* prototypes for system CPU information functions */
void ReadCPUInformation(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* Macros for Memory information */
#define Natts_memory_info       8
#define MEMORY_FILE_NAME        "/proc/meminfo"
#define Anum_total_memory       0
#define Anum_free_memory        1
#define Anum_available_memory   2
#define Anum_buffers            3
#define Anum_cached             4
#define Anum_swap_cached        5
#define Anum_swap_total         6
#define Anum_swap_free          7

/* prototypes for system memory information functions */
uint64_t ConvertToBytes(char *line_buf);
void ReadMemoryInformation(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* Macros for load average information */
#define Natts_load_avg_info                    3
#define CPU_IO_LOAD_AVG_FILE                   "/proc/loadavg"
#define Anum_cpu_io_load_avg_one_minute        0
#define Anum_cpu_io_load_avg_five_minutes      1
#define Anum_cpu_io_load_avg_ten_minutes       2

/* prototypes for system load average information functions */
void ReadLoadAvgInformations(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* Macros for operating system information */
#define Natts_os_info                7
#define OS_INFO_FILE_NAME            "/etc/os-release"
#define OS_DESC_SEARCH_TEXT          "PRETTY_NAME="
#define OS_VERSION_SEARCH_TEXT       "VERSION_ID="
#define OS_CODE_NAME_SEARCH_TEXT     "CODENAME="
#define Anum_host_name               0
#define Anum_domain_name             1
#define Anum_kernel_info             2
#define Anum_architecture            3
#define Anum_os_description          4
#define Anum_os_release_version      5
#define Anum_os_codename             6

/* prototypes for operating system information functions */
void ReadOSInformations(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* Macros for system CPU usage information */
#define Natts_cpu_usage_stats             8
#define CPU_USAGE_STATS_FILENAME          "/proc/stat"
#define Anum_cpu_name                     0
#define Anum_usermode_normal_process      1
#define Anum_usermode_niced_process       2
#define Anum_kernelmode_process           3
#define Anum_idle_mode                    4
#define Anum_io_completion                5
#define Anum_servicing_irq                6
#define Anum_servicing_softirq            7

/* prototypes for system CPU usage information functions */
void ReadCPUUsageStatistics(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* Macros for system processes information */
#define Natts_process_info         5
#define Anum_active_processes      0
#define Anum_running_processes     1
#define Anum_sleeping_processes    2
#define Anum_stopped_processes     3
#define Anum_zombie_processes      4

/* prototypes for system process information functions */
void ReadProcessInformations(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* Macros for network information */
#define Natts_network_info          12
#define Anum_net_interface_name     0
#define Anum_net_ipv4_address       1
#define Anum_net_ipv6_address       2
#define Anum_net_speed_mbps         3
#define Anum_net_tx_bytes           4
#define Anum_net_tx_packets         5
#define Anum_net_tx_errors          6
#define Anum_net_tx_dropped         7
#define Anum_net_rx_bytes           8
#define Anum_net_rx_packets         9
#define Anum_net_rx_errors          10
#define Anum_net_rx_dropped         11

/* prototypes for system network information functions */
void ReadNetworkInformations(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* Macros for cpu and memory information by process*/
#define Natts_cpu_memory_info_by_process     4
#define Anum_process_pid                     0
#define Anum_process_command                 1
#define Anum_process_cpu_usage               2
#define Anum_process_memory_usage            3

/* prototypes for system network information functions */
void ReadCPUMemoryByProcess(Tuplestorestate *tupstore, TupleDesc tupdesc);

#endif // SYSTEM_STATS_H
