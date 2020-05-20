/*------------------------------------------------------------------------
 * network_info.c
 *              System network information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"

#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#define IP_ADDR_SIZE 20

typedef struct iprow
{
	int indx;
	char addr[IP_ADDR_SIZE];
} IPROW;

void ReadNetworkInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum            values[Natts_network_info];
	bool             nulls[Natts_network_info];
	int              bufsize = 0;
	int              retval = 0;
	int              allocate_buffer_cnt = 0;
	PMIB_IPADDRTABLE ip_addr_table;
	MIB_IFTABLE      *if_table;
	IPROW            *ip_rows;

	memset(nulls, 0, sizeof(nulls));

	// Populate the address table data-structure
	ip_addr_table = (MIB_IPADDRTABLE *)malloc(sizeof(MIB_IPADDRTABLE));
	if (ip_addr_table == NULL)
	{
		ereport(DEBUG1, (errmsg("[ReadNetworkInformations]: Failed to allocate the memory")));
		return;
	}

	/* Make an initial call to GetIpAddrTable to get the necessary size into the bufsize variable */
	if (GetIpAddrTable(ip_addr_table, (PULONG)&bufsize, 0) == ERROR_INSUFFICIENT_BUFFER)
	{
		free(ip_addr_table);
		ip_addr_table = NULL;
		ip_addr_table = (MIB_IPADDRTABLE *)malloc(bufsize);
		if (ip_addr_table == NULL)
		{
			ereport(DEBUG1, (errmsg("[ReadNetworkInformations]: Failed to allocate the memory")));
			return;
		}
	}

	/* Make a second call to GetIpAddrTable to get the actual data */
	if ((retval = GetIpAddrTable(ip_addr_table, (PULONG)&bufsize, 0)) != NO_ERROR)
	{
		ereport(DEBUG1, (errmsg("[ReadNetworkInformations]: GetIpAddrTable: Failed to get information")));
		free(ip_addr_table);
		return;
	}

	/* First find the buffer size for allocation */
	for (int iter = 0; iter < (int)ip_addr_table->dwNumEntries; iter++)
	{
		if ((ip_addr_table->table[iter].wType & MIB_IPADDR_PRIMARY) || (ip_addr_table->table[iter].wType & MIB_IPADDR_DYNAMIC))
			allocate_buffer_cnt++;
	}

	if (allocate_buffer_cnt != 0)
	{
		ip_rows = (IPROW *)malloc(allocate_buffer_cnt * sizeof(IPROW));
		if (ip_rows == NULL)
		{
			ereport(DEBUG1, (errmsg("[ReadNetworkInformations]: Failed to allocate the memory")));
			return;
		}

		memset(ip_rows, 0x00, allocate_buffer_cnt * sizeof(IPROW));
	}
	else
	{
		ereport(DEBUG1, (errmsg("[ReadNetworkInformations]: No valid interface available")));
		return;
	}

	for (int iter = 0; iter < (int)ip_addr_table->dwNumEntries; iter++)
	{
		if ((ip_addr_table->table[iter].wType & MIB_IPADDR_PRIMARY) || (ip_addr_table->table[iter].wType & MIB_IPADDR_DYNAMIC))
		{
			int index = sizeof(IPROW) * iter;
			int ip_index = index + sizeof(int);
			IN_ADDR IPAddr;

			int val = ip_addr_table->table[iter].dwIndex;
			IPAddr.S_un.S_addr = (u_long)ip_addr_table->table[iter].dwAddr;
			char *ip_val = inet_ntoa(IPAddr);
			memcpy((ip_rows + index), (int *)&val, sizeof(int));
			memcpy((ip_rows + ip_index), (char *)ip_val, IP_ADDR_SIZE);
		}
	}

	free(ip_addr_table);
	ip_addr_table = NULL;

	/* Populate the interface stat table data-structure */
	if_table = (MIB_IFTABLE *)malloc(sizeof(MIB_IFTABLE));
	if (if_table == NULL)
	{
		ereport(DEBUG1, (errmsg("[ReadNetworkInformations]: Failed to allocate the memory")));
		return;
	}

	/* Make an initial call to GetIfTable to get the necessary size into bufsize */
	if (GetIfTable(if_table, (PULONG)&bufsize, FALSE) == ERROR_INSUFFICIENT_BUFFER)
	{
		free(if_table);
		if_table = (MIB_IFTABLE *)malloc(bufsize);
		if (if_table == NULL)
		{
			ereport(DEBUG1, (errmsg("[ReadNetworkInformations]: Failed to allocate the memory")));
			return;
		}
	}

	/* Make a second call to GetIfTable to get the actual data */
	if ((retval = GetIfTable(if_table, (PULONG)&bufsize, FALSE)) != NO_ERROR)
	{
		ereport(DEBUG1, (errmsg("[ReadNetworkInformations]: GetIfTable: Failed to get information")));
		free(if_table);
		return;
	}

	for (int o_index = 0; o_index < (int)allocate_buffer_cnt; o_index++)
	{
		for (int i_index = 0; i_index < (int)if_table->dwNumEntries; i_index++)
		{
			MIB_IFROW *if_row = (MIB_IFROW *)& if_table->table[i_index];
			int index = sizeof(IPROW) * o_index;
			int ip_index = index + sizeof(int);

			int val = *(int*)(ip_rows + index);

			if (val == if_row->dwIndex)
			{
				if (if_row->dwType == IF_TYPE_ETHERNET_CSMACD ||
					if_row->dwType == IF_TYPE_IEEE80211 ||
					if_row->dwType == IF_TYPE_SOFTWARE_LOOPBACK ||
					if_row->dwType == IF_TYPE_IEEE1394)
				{
					bool IsLoopbackType = false;
					if (if_row->dwType == IF_TYPE_SOFTWARE_LOOPBACK)
						IsLoopbackType = true;

					/* if the interface is not valid */
					if (IsLoopbackType == false &&
						(if_row->dwOperStatus != IF_OPER_STATUS_OPERATIONAL ||
							if_row->dwPhysAddrLen == 0))
						continue;

					int     wstr_length = 0;
					char    *dst = NULL;
					size_t  charsConverted = 0;

					wstr_length = (int)wcslen(if_row->wszName);
					if (wstr_length == 0)
						nulls[Anum_net_interface_name] = true;
					else
					{
						dst = (char *)malloc(wstr_length + 10);
						memset(dst, 0x00, (wstr_length + 10));
						wcstombs_s(&charsConverted, dst, wstr_length + 10, if_row->wszName, wstr_length);
						values[Anum_net_interface_name] = CStringGetTextDatum(dst);
						free(dst);
					}

					values[Anum_net_ipv4_address] = CStringGetTextDatum((char *)(ip_rows + ip_index));
					values[Anum_net_tx_packets] = Int64GetDatumFast((uint64)(if_row->dwOutUcastPkts + if_row->dwOutNUcastPkts));
					values[Anum_net_rx_packets] = Int64GetDatumFast((uint64)(if_row->dwInUcastPkts + if_row->dwInNUcastPkts));
					values[Anum_net_tx_bytes] = Int64GetDatumFast((uint64)if_row->dwOutOctets);
					values[Anum_net_rx_bytes] = Int64GetDatumFast((uint64)if_row->dwInOctets);
					values[Anum_net_tx_dropped] = Int64GetDatumFast((uint64)if_row->dwOutDiscards);
					values[Anum_net_rx_dropped] = Int64GetDatumFast((uint64)if_row->dwInDiscards);
					values[Anum_net_tx_errors] = Int64GetDatumFast((uint64)if_row->dwOutErrors);
					values[Anum_net_rx_errors] = Int64GetDatumFast((uint64)if_row->dwInErrors);
					values[Anum_net_speed_mbps] = Int32GetDatum((int)(if_row->dwSpeed / 1000000));

					tuplestore_putvalues(tupstore, tupdesc, values, nulls);
				}
			}
		}
	}

	free(if_table);
	if_table = NULL;
}