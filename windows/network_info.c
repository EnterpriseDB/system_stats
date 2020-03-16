/*------------------------------------------------------------------------
 * network_info.c
 *              System network information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "stats.h"

void ReadNetworkInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	//TODO
	// Win32_NetworkAdapter, Win32_PerfFormattedData_Tcpip_NetworkInterface, Win32_NetworkAdapterConfiguration
}