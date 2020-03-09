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

#endif // STATS_H