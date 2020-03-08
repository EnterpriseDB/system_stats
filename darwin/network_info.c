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

#include <unistd.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>
#include <sys/sysctl.h>
#include <sys/types.h>

#include <sys/vmmeter.h>
#include <libproc.h>
#include <sys/proc_info.h>
#include <netinet/tcp_fsm.h>
#include <arpa/inet.h>
#include <net/if_dl.h>

#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>

void ReadNetworkInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum      values[Natts_network_info];
	bool       nulls[Natts_network_info];
	char       interface_name[MAXPGPATH];
	char       ipv4_address[MAXPGPATH];
	uint64     speed_mbps = 0;
	uint64     tx_bytes = 0;
	uint64     tx_packets = 0;
	uint64     tx_errors = 0;
	uint64     tx_dropped = 0;
	uint64     rx_bytes = 0;
	uint64     rx_packets = 0;
	uint64     rx_errors = 0;
	uint64     rx_dropped = 0;

	// First find out interface and ip address of that interface
	struct ifaddrs *ifaddr;
	struct ifaddrs *ifa;
	int ret_val;
	char host[MAXPGPATH];

    // networking subsystem initialization
    char *buf = NULL, *limit, *next_ptr;
    struct if_msghdr *ifm;
    size_t len;
    int desc[6];
    desc[0] = CTL_NET;
    desc[1] = PF_ROUTE;
    desc[2] = 0;
    desc[3] = 0;
    desc[4] = NET_RT_IFLIST2;
    desc[5] = 0;
 
	memset(nulls, 0, sizeof(nulls));
	memset(interface_name, 0, MAXPGPATH);
	memset(ipv4_address, 0, MAXPGPATH);
	memset(host, 0, MAXPGPATH);

    if (sysctl(desc, 6, NULL, &len, NULL, 0) < 0)
    {
		ereport(DEBUG1,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("Failed to get networking subsystem parameters")));
        return;
    }
 
    buf = malloc(len);
    if (buf == NULL)
    {
		ereport(DEBUG1, (errmsg("Failed to allocate the memory")));
        return;
    }
 
    if (sysctl(desc, 6, buf, &len, NULL, 0) < 0)
    {
		ereport(DEBUG1,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("Failed to get networking subsystem parameters")));
        if (buf)
            free(buf);
        return;
    }
 
    limit = buf + len;

	/* Below function is used to creates a linked list of structures describing
	 * the network interfaces of the local system
	 */
	if (getifaddrs(&ifaddr) == -1)
	{
		ereport(DEBUG1,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("Failed to get network interface")));
        if (buf)
            free(buf);
        return;
	}

    for (next_ptr = buf; next_ptr < limit;)
    {
        ifm = (struct if_msghdr *)next_ptr;
        next_ptr += ifm->ifm_msglen;

        if (ifm->ifm_type == RTM_IFINFO2)
        {
            struct if_msghdr2 *if2m = (struct if_msghdr2 *)ifm;
            struct sockaddr_dl *sdl = (struct sockaddr_dl *)(if2m + 1);

            strncpy(interface_name, sdl->sdl_data, sdl->sdl_nlen);
            interface_name[sdl->sdl_nlen] = 0;

            for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
            {
                if (ifa->ifa_addr == NULL)
                    continue;

                ret_val = getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

                if((strcmp(ifa->ifa_name, interface_name)==0) && (ifa->ifa_addr->sa_family==AF_INET))
                {
                    if (ret_val != 0)
                    {
				        ereport(ERROR,
					        (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						        errmsg("getnameinfo() failed: %s", gai_strerror(ret_val))));
				        nulls[Anum_net_ipv4_address] = true;
                    }
                    memcpy(ipv4_address, host, MAXPGPATH);
                    elog(WARNING, "[%s][%lld][%lld][%lld][%lld][%lld][%lld][%lld]\n", interface_name, if2m->ifm_data.ifi_obytes, if2m->ifm_data.ifi_ibytes, if2m->ifm_data.ifi_opackets, if2m->ifm_data.ifi_ipackets, if2m->ifm_data.ifi_ierrors, if2m->ifm_data.ifi_oerrors, if2m->ifm_data.ifi_iqdrops);

                    // Assign the values
                    tx_bytes = (uint64)if2m->ifm_data.ifi_obytes;
                    tx_packets = (uint64)if2m->ifm_data.ifi_opackets;
                    tx_errors = (uint64)if2m->ifm_data.ifi_oerrors;
                    tx_dropped = (uint64)0;
                    speed_mbps = (uint64)0;
                    rx_bytes = (uint64)if2m->ifm_data.ifi_ibytes;
                    rx_packets = (uint64)if2m->ifm_data.ifi_ipackets;
                    rx_errors = (uint64)if2m->ifm_data.ifi_ierrors;
                    rx_dropped = (uint64)if2m->ifm_data.ifi_iqdrops;

			        values[Anum_net_interface_name] = CStringGetTextDatum(interface_name);
			        values[Anum_net_ipv4_address] = CStringGetTextDatum(ipv4_address);
			        values[Anum_net_speed_mbps] = Int64GetDatumFast(speed_mbps);
			        values[Anum_net_tx_bytes] = Int64GetDatumFast(tx_bytes);
			        values[Anum_net_tx_packets] = Int64GetDatumFast(tx_packets);
			        values[Anum_net_tx_errors] = Int64GetDatumFast(tx_errors);
			        values[Anum_net_tx_dropped] = Int64GetDatumFast(tx_dropped);
			        values[Anum_net_rx_bytes] = Int64GetDatumFast(rx_bytes);
			        values[Anum_net_rx_packets] = Int64GetDatumFast(rx_packets);
			        values[Anum_net_rx_errors] = Int64GetDatumFast(rx_errors);
			        values[Anum_net_rx_dropped] = Int64GetDatumFast(rx_dropped);

			        tuplestore_putvalues(tupstore, tupdesc, values, nulls);

			        //reset the value again
			        memset(interface_name, 0, MAXPGPATH);
			        memset(ipv4_address, 0, MAXPGPATH);
			        speed_mbps = 0;
			        tx_bytes = 0;
			        tx_packets = 0;
			        tx_errors = 0;
			        tx_dropped = 0;
			        rx_bytes = 0;
			        rx_packets = 0;
			        rx_errors = 0;
			        rx_dropped = 0;
                }
            }
        }
        else
            continue;
    }

    freeifaddrs(ifaddr);
    free(buf);
}
