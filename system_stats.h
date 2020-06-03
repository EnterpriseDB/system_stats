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

#ifdef WIN32
#include <windows.h>
#include <wbemidl.h>
#endif

#include "access/tupdesc.h"
#include "utils/tuplestore.h"
#include "utils/builtins.h"

/* prototypes for system disk information functions */
void ReadDiskInformation(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for system IO analysis functions */
void ReadIOAnalysisInformation(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for system CPU information functions */
void ReadCPUInformation(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for system memory information functions */
void ReadMemoryInformation(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for system load average information functions */
void ReadLoadAvgInformations(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for operating system information functions */
void ReadOSInformations(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for system CPU usage information functions */
void ReadCPUUsageStatistics(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for system process information functions */
void ReadProcessInformations(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for system network information functions */
void ReadNetworkInformations(Tuplestorestate *tupstore, TupleDesc tupdesc);

/* prototypes for system network information functions */
void ReadCPUMemoryByProcess(Tuplestorestate *tupstore, TupleDesc tupdesc);

#ifndef WIN32
/* prototypes for common string manipulations and command execution functions */
bool stringIsNumber(char *str);
char* lefttrimStr(char *);
char* righttrimStr(char *);
char*  trimStr(char *);
float fl_round(float val);
bool read_process_status(int *active_processes, int *running_processes,
		int *sleeping_processes, int *stopped_processes, int *zombie_processes, int *total_threads);
void ReadFileContent(const char *file_name, uint64 *data);

/* prototypes for system disk information functions */
bool ignoreFileSystemTypes(char *fs_mnt);
bool ignoreMountPoints(char *fs_mnt);

/* prototypes for system memory information functions */
uint64_t ConvertToBytes(char *line_buf);
#else
void initialize_wmi_connection();
void uninitialize_wmi_connection();
void execute_init_query();
IEnumWbemClassObject* execute_query(BSTR query);
int is_process_running(int pid);
#endif

/* read the the output of command in chunk of 1024 bytes */
#define READ_CHUNK_BYTES     1024
#define MIN_BUFFER_SIZE      512
#define MAX_BUFFER_SIZE      2048
#define IS_EMPTY_STR(X) ((1 / (sizeof(X[0]) == 1)) && !(X[0]))
#define PROC_FILE_SYSTEM_PATH    "/proc"

/* Macros for system disk information */
#define Natts_disk_info                          11
#define FILE_SYSTEM_MOUNT_FILE_NAME              "/etc/mtab"
#define IGNORE_MOUNT_POINTS_REGEX                "^/(dev|proc|sys|run|snap|var/lib/docker/.+)($|/)"
#define IGNORE_FILE_SYSTEM_TYPE_REGEX            "^(autofs|binfmt_misc|bpf|cgroup2?|configfs|debugfs|devpts|devtmpfs|fusectl|hugetlbfs|iso9660|mqueue|nsfs|overlay|proc|procfs|pstore|rpc_pipefs|securityfs|selinuxfs|squashfs|sysfs|tracefs)$"
#define Anum_disk_mount_point                    0
#define Anum_disk_file_system                    1
#define Anum_disk_drive_letter                   2
#define Anum_disk_drive_type                     3
#define Anum_disk_file_system_type               4
#define Anum_disk_total_space                    5
#define Anum_disk_used_space                     6
#define Anum_disk_free_space                     7
#define Anum_disk_total_inodes                   8
#define Anum_disk_used_inodes                    9
#define Anum_disk_free_inodes                    10

/* Macros for system IO Analysis */
#define Natts_io_analysis_info                   7
#define DISK_IO_STATS_FILE_NAME                  "/proc/diskstats"
#define Anum_device_name                         0
#define Anum_total_read                          1
#define Anum_total_write                         2
#define Anum_read_bytes                          3
#define Anum_write_bytes                         4
#define Anum_read_time_ms                        5
#define Anum_write_time_ms                       6

/* Macros for system CPU information */
#define Natts_cpu_info                           16
#define CPU_INFO_FILE_NAME                       "/proc/cpuinfo"
#define Anum_cpu_vendor                          0
#define Anum_cpu_description                     1
#define Anum_model_name                          2
#define Anum_processor_type                      3
#define Anum_logical_processor                   4
#define Anum_physical_processor                  5
#define Anum_no_of_cores                         6
#define Anum_architecture                        7
#define Anum_cpu_clock_speed                     8
#define Anum_cpu_type                            9
#define Anum_cpu_family                          10
#define Anum_cpu_byte_order                      11
#define Anum_l1dcache_size                       12
#define Anum_l1icache_size                       13
#define Anum_l2cache_size                        14
#define Anum_l3cache_size                        15

/* Macros for Memory information */
#define MEMORY_READ_COUNT                        5
#define Natts_memory_info                        12
#define MEMORY_FILE_NAME                         "/proc/meminfo"
#define Anum_total_memory                        0
#define Anum_used_memory                         1
#define Anum_free_memory                         2
#define Anum_swap_total_memory                   3
#define Anum_swap_used_memory                    4
#define Anum_swap_free_memory                    5
#define Anum_total_cache_memory                  6
#define Anum_kernel_total_memory                 7
#define Anum_kernel_paged_memory                 8
#define Anum_kernel_nonpaged_memory              9
#define Anum_total_page_file                     10
#define Anum_avail_page_file                     11

/* Macros for load average information */
#define CPU_IO_LOAD_AVG_FILE                     "/proc/loadavg"
#define Natts_load_avg_info                      4
#define Anum_load_avg_one_minute                 0
#define Anum_load_avg_five_minutes               1
#define Anum_load_avg_ten_minutes                2
#define Anum_load_avg_fifteen_minutes            3

/* Macros for operating system information */
#define Natts_os_info                            10
#define OS_INFO_FILE_NAME                        "/etc/os-release"
#define OS_DESC_SEARCH_TEXT                      "PRETTY_NAME="
#define OS_HANDLE_READ_FILE_PATH                 "/proc/sys/fs/file-nr"
#define OS_BOOT_UP_SINCE_FILE_PATH               "/proc/uptime"
#define Anum_os_name                             0
#define Anum_os_version                          1
#define Anum_host_name                           2
#define Anum_domain_name                         3
#define Anum_os_handle_count                     4
#define Anum_os_process_count                    5
#define Anum_os_thread_count                     6
#define Anum_os_architecture                     7
#define Anum_os_boot_time                        8
#define Anum_os_up_since_seconds                 9

/* Macros for system CPU usage information */
#define Natts_cpu_usage_stats                    11
#define CPU_USAGE_STATS_FILENAME                 "/proc/stat"
#define Anum_usermode_normal_process             0
#define Anum_usermode_niced_process              1
#define Anum_kernelmode_process                  2
#define Anum_idle_mode                           3
#define Anum_io_completion                       4
#define Anum_servicing_irq                       5
#define Anum_servicing_softirq                   6
#define Anum_percent_user_time                   7
#define Anum_percent_processor_time              8
#define Anum_percent_privileged_time             9
#define Anum_percent_interrupt_time              10

/* Macros for system processes information */
#define Natts_process_info                       5
#define Anum_no_of_total_processes               0
#define Anum_no_of_running_processes             1
#define Anum_no_of_sleeping_processes            2
#define Anum_no_of_stopped_processes             3
#define Anum_no_of_zombie_processes              4

/* Macros for network information */
#define Natts_network_info                       11
#define Anum_net_interface_name                  0
#define Anum_net_ipv4_address                    1
#define Anum_net_tx_bytes                        2
#define Anum_net_tx_packets                      3
#define Anum_net_tx_errors                       4
#define Anum_net_tx_dropped                      5
#define Anum_net_rx_bytes                        6
#define Anum_net_rx_packets                      7
#define Anum_net_rx_errors                       8
#define Anum_net_rx_dropped                      9
#define Anum_net_speed_mbps                      10

/* Macros for cpu and memory information
 * by process*/
#define Natts_cpu_memory_info_by_process         6
#define Anum_process_pid                         0
#define Anum_process_name                        1
#define Anum_process_running_since               2
#define Anum_percent_cpu_usage                   3
#define Anum_percent_memory_usage                4
#define Anum_process_memory_bytes                5

#endif // SYSTEM_STATS_H
