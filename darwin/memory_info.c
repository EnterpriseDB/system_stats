/*------------------------------------------------------------------------
 * memory_info.c
 *              System memory information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */
#include "postgres.h"
#include "system_stats.h"

#include <sys/types.h>
#include <sys/sysctl.h>
#include <unistd.h>

#include <mach/mach.h>
#include <mach/vm_page_size.h>

void ReadMemoryInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum      values[Natts_memory_info];
	bool       nulls[Natts_memory_info];
	uint64     total_memory_bytes = 0;
	uint64     free_memory_bytes = 0;
	uint64     used_memory_bytes = 0;
	vm_statistics64_data_t  vm_stat;
	kern_return_t ret;
	mach_msg_type_number_t count = sizeof(vm_stat) / sizeof(int);
	size_t    size;
	struct    xsw_usage swap_mem;
	int       desc[2];
	uint64_t  total;
	size_t    len = sizeof(total);
	int       pagesize = getpagesize();
      
	memset(nulls, 0, sizeof(nulls));

	// Set the read parameter from kernel as virtual memory total size
	desc[0] = CTL_HW;
	desc[1] = HW_MEMSIZE;
      
	if (sysctl(desc, 2, &total_memory_bytes, &len, NULL, 0))
	{
		ereport(DEBUG1, (errmsg("Error while getting total memory information")));
		return;
	}
      
	mach_port_t mport = mach_host_self();

	/* Read the host memory statistics */
	ret = host_statistics(mport, HOST_VM_INFO, (host_info_t)&vm_stat, &count);
	if (ret != KERN_SUCCESS) {
		ereport(DEBUG1, (errmsg("host_statistics failed to get the host information")));
	}

	mach_port_deallocate(mach_task_self(), mport);
      
	free_memory_bytes = (vm_stat.inactive_count + vm_stat.free_count) * pagesize;
	used_memory_bytes = total_memory_bytes - free_memory_bytes;
     
	/* Set the read parameter for swap memory usage */ 
	desc[0] = CTL_VM;
	desc[1] = VM_SWAPUSAGE;
	size = sizeof(swap_mem);

	/* Read the swap memory usage */
	if (sysctl(desc, 2, &swap_mem, &size, NULL, 0) == -1)
	{
		ereport(DEBUG1, (errmsg("Error while getting swap memory information")));
	}

	values[Anum_total_memory] = Int64GetDatumFast(total_memory_bytes);
	values[Anum_used_memory] = Int64GetDatumFast(used_memory_bytes);
	values[Anum_free_memory] = Int64GetDatumFast(free_memory_bytes);
	values[Anum_swap_total_memory] = Int64GetDatumFast(swap_mem.xsu_total);
	values[Anum_swap_used_memory] = Int64GetDatumFast(swap_mem.xsu_used);
	values[Anum_swap_free_memory] = Int64GetDatumFast(swap_mem.xsu_avail);
	nulls[Anum_total_cache_memory] = true;
	nulls[Anum_kernel_total_memory] = true;
	nulls[Anum_kernel_paged_memory] = true;
	nulls[Anum_kernel_nonpaged_memory] = true;
	nulls[Anum_total_page_file] = true;
	nulls[Anum_avail_page_file] = true;

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);
}
