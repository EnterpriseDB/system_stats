/*------------------------------------------------------------------------
 * system_stats_utils.c
 *              Defined required utility functions to fetch
 *              system statistics information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */
#include "postgres.h"
#include "stats.h"

#include <sys/types.h>
#include <sys/sysctl.h>

#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/mach_host.h>

int get_process_list(struct kinfo_proc **proc_list, size_t *proc_count);

uint64 find_cpu_times(void);

/* Below function is used to get cpu times for all the cpu states */
uint64 find_cpu_times()
{
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    kern_return_t error;
    host_cpu_load_info_data_t r_load;
    uint64 total_time;

    mach_port_t host_port = mach_host_self();
    error = host_statistics(host_port, HOST_CPU_LOAD_INFO,
                            (host_info_t)&r_load, &count);
    if (error != KERN_SUCCESS)
    {
        ereport(DEBUG1, (errmsg("host_statistics syscall failed: %s", mach_error_string(error))));
        return (uint64)0;
    }

    mach_port_deallocate(mach_task_self(), host_port);

    total_time = r_load.cpu_ticks[CPU_STATE_USER] + r_load.cpu_ticks[CPU_STATE_NICE] +
    r_load.cpu_ticks[CPU_STATE_SYSTEM] + r_load.cpu_ticks[CPU_STATE_IDLE];

    return total_time;
 }

/* Below function is used to get all the process information and their status */
int get_process_list(struct kinfo_proc **proc_list, size_t *proc_count)
{
    int mib_desc[3] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL };
    size_t size, new_size;
    void   *proc_list_ptr;

    *proc_count = 0;

    size = 0;
    if (sysctl((int *)mib_desc, 3, NULL, &size, NULL, 0) == -1)
    {
        ereport(DEBUG1, (errmsg("Failed to get process informations")));
        return 1;
    }

    new_size = size + (size >> 3);
    if (new_size > size)
    {
        proc_list_ptr = malloc(new_size);
        if (proc_list_ptr == NULL)
            proc_list_ptr = malloc(size);
        else
            size = new_size;
    }
    else
        proc_list_ptr = malloc(size);

    if (proc_list_ptr == NULL)
    {
        ereport(DEBUG1, (errmsg("Failed to allocate the memory")));
        return 1;
    }

    if (sysctl((int *)mib_desc, 3, proc_list_ptr, &size, NULL, 0) == -1)
    {
        if (proc_list_ptr)
            free(proc_list_ptr);
        ereport(DEBUG1, (errmsg("Failed to get process informations")));
        return 1;
    }
    else
    {
        *proc_list = (struct kinfo_proc *)proc_list_ptr;
        *proc_count = size / sizeof(struct kinfo_proc);
        if (proc_count <= 0)
        {
            if (proc_list_ptr)
                free(proc_list_ptr);
            ereport(DEBUG1, (errmsg("Process count is zero")));
            return 1;
        }

        /* return with success as it got process information */
        return 0;
    }

    if (proc_list_ptr)
        free(proc_list_ptr);

    return 1;
}
