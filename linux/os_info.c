/*------------------------------------------------------------------------
 * os_info.c
 *              Operating system information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"
#include "misc.h"

#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

bool total_opened_handle(int *total_handles);
static bool get_dns_domain_name(const char *hostname, char *domain_name, size_t domain_size);
void ReadOSInformations(Tuplestorestate *tupstore, TupleDesc tupdesc);

bool total_opened_handle(int *total_handles)
{
	FILE          *fp;
	char          *line_buf = NULL;
	size_t        line_buf_size = 0;
	ssize_t       line_size;
	int           allocated_handle_count;
	int           unallocated_handle_count;
	int           max_handle_count;
	const char    *scan_fmt = "%d %d %d";

	fp = fopen(OS_HANDLE_READ_FILE_PATH, "r");

	if (!fp)
	{
		ereport(DEBUG1, (errmsg("can not open file for reading handle informations")));
		return false;
	}

	/* Get the first line of the file. */
	line_size = getline(&line_buf, &line_buf_size, fp);

	/* Loop through until we are done with the file. */
	if (line_size >= 0)
		sscanf(line_buf, scan_fmt, &allocated_handle_count, &unallocated_handle_count, &max_handle_count);

	if (line_buf != NULL)
	{
		free(line_buf);
		line_buf = NULL;
	}

	fclose(fp);

	*total_handles = allocated_handle_count;

	return true;
}

/*
 * get_dns_domain_name
 *
 * Attempts to retrieve the DNS domain name using multiple fallback strategies:
 * 1. Try DNS resolution to get FQDN, then extract domain
 * 2. Read from /etc/resolv.conf (domain or search directive)
 * 3. Fallback to getdomainname() with validation
 *
 * Returns true if domain name was found, false otherwise
 */
static bool get_dns_domain_name(const char *hostname, char *domain_name, size_t domain_size)
{
	struct addrinfo hints, *info = NULL;
	bool found = false;
	char *dot_pos = NULL;

	memset(domain_name, 0, domain_size);

	/* Strategy 1: Try DNS resolution to get FQDN */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;

	if (getaddrinfo(hostname, NULL, &hints, &info) == 0 && info != NULL)
	{
		if (info->ai_canonname != NULL)
		{
			/* Look for first dot to extract domain from FQDN */
			dot_pos = strchr(info->ai_canonname, '.');
			if (dot_pos != NULL && strlen(dot_pos + 1) > 0)
			{
				/* Copy domain part (after first dot) */
				snprintf(domain_name, domain_size, "%s", dot_pos + 1);
				found = true;
				ereport(DEBUG1, (errmsg("domain name extracted from FQDN: %s", domain_name)));
			}
		}
		freeaddrinfo(info);
	}

	/* Strategy 2: If DNS failed, try reading from /etc/resolv.conf */
	if (!found)
	{
		FILE *resolv_file = fopen("/etc/resolv.conf", "r");
		if (resolv_file != NULL)
		{
			char *line_buf = NULL;
			size_t line_buf_size = 0;
			ssize_t line_size;

			while ((line_size = getline(&line_buf, &line_buf_size, resolv_file)) >= 0)
			{
				char *trimmed = str_trim(line_buf);

				/* Check for "domain" directive (preferred) */
				if (strncmp(trimmed, "domain", 6) == 0)
				{
					char *domain_val = str_trim(trimmed + 6);
					if (strlen(domain_val) > 0)
					{
						snprintf(domain_name, domain_size, "%s", domain_val);
						found = true;
						ereport(DEBUG1, (errmsg("domain name from /etc/resolv.conf (domain): %s", domain_name)));
						break;
					}
				}
				/* Check for "search" directive (fallback) */
				else if (!found && strncmp(trimmed, "search", 6) == 0)
				{
					char *search_val = str_trim(trimmed + 6);
					/* Take first domain from search list */
					char *space = strchr(search_val, ' ');
					if (space != NULL)
						*space = '\0';

					if (strlen(search_val) > 0)
					{
						snprintf(domain_name, domain_size, "%s", search_val);
						found = true;
						ereport(DEBUG1, (errmsg("domain name from /etc/resolv.conf (search): %s", domain_name)));
					}
				}
			}

			if (line_buf != NULL)
				free(line_buf);
			fclose(resolv_file);
		}
	}

	/* Strategy 3: Last resort - try getdomainname() with validation */
	if (!found)
	{
		char nis_domain[256];
		memset(nis_domain, 0, sizeof(nis_domain));

		if (getdomainname(nis_domain, sizeof(nis_domain)) == 0)
		{
			/* Validate that it's not empty or "(none)" */
			if (strlen(nis_domain) > 0 &&
				strcmp(nis_domain, "(none)") != 0 &&
				strcmp(nis_domain, "localdomain") != 0)
			{
				snprintf(domain_name, domain_size, "%s", nis_domain);
				found = true;
				ereport(DEBUG1, (errmsg("domain name from getdomainname(): %s", domain_name)));
			}
		}
	}

	return found;
}

void ReadOSInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	struct     utsname uts;
	struct     sysinfo s_info;
	Datum      values[Natts_os_info];
	bool       nulls[Natts_os_info];
	char       host_name[MAXPGPATH];
	char       domain_name[MAXPGPATH];
	char       version[MAXPGPATH];
	char       architecture[MAXPGPATH];
	char       os_name[MAXPGPATH];
	int        ret_val;
	FILE       *os_info_file;
	char       *line_buf = NULL;
	size_t     line_buf_size = 0;
	ssize_t    line_size;
	int        active_processes = 0;
	int        running_processes = 0;
	int        sleeping_processes = 0;
	int        stopped_processes = 0;
	int        zombie_processes = 0;
	int        total_threads = 0;
	int        handle_count = 0;

	memset(nulls, 0, sizeof(nulls));
	memset(host_name, 0, MAXPGPATH);
	memset(domain_name, 0, MAXPGPATH);
	memset(version, 0, MAXPGPATH);
	memset(architecture, 0, MAXPGPATH);
	memset(os_name, 0, MAXPGPATH);

	ret_val = uname(&uts);
	/* if it returns not zero means it fails so set null values */
	if (ret_val != 0)
	{
		nulls[Anum_os_version]  = true;
		nulls[Anum_architecture] = true;
	}
	else
	{
		snprintf(version, MAXPGPATH, "%s %s", uts.sysname, uts.release);
		memcpy(architecture, uts.machine, strlen(uts.machine));
	}

	/* Function used to get the host name of the system */
	if (gethostname(host_name, sizeof(host_name)) != 0)
	{
		ereport(DEBUG1,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("error while getting host name")));
		nulls[Anum_host_name] = true;
	}
	else if (strlen(host_name) == 0)
	{
		nulls[Anum_host_name] = true;
	}

	/* Get DNS domain name using multiple fallback strategies */
	if (!get_dns_domain_name(host_name, domain_name, sizeof(domain_name)))
	{
		/* All strategies failed, set to NULL */
		nulls[Anum_domain_name] = true;
		ereport(DEBUG1,
				(errmsg("unable to determine domain name from any source")));
	}

	os_info_file = fopen(OS_INFO_FILE_NAME, "r");

	if (!os_info_file)
	{
		char os_info_file_name[MAXPGPATH];
		snprintf(os_info_file_name, MAXPGPATH, "%s", OS_INFO_FILE_NAME);

		ereport(DEBUG1,
				(errcode_for_file_access(),
					errmsg("can not open file %s for reading os information",
						os_info_file_name)));

		nulls[Anum_os_name] = true;
	}
	else
	{
		/* Get the first line of the file. */
		line_size = getline(&line_buf, &line_buf_size, os_info_file);

		/* Loop through until we are done with the file. */
		while (line_size >= 0)
		{
			int len = strlen(line_buf);
			if (strstr(line_buf, OS_DESC_SEARCH_TEXT) != NULL)
				memcpy(os_name, remove_quotes(str_trim(line_buf + strlen(OS_DESC_SEARCH_TEXT))), (len - strlen(OS_DESC_SEARCH_TEXT)));

			/* Free the allocated line buffer */
			if (line_buf != NULL)
			{
				free(line_buf);
				line_buf = NULL;
			}

			/* Get the next line */
			line_size = getline(&line_buf, &line_buf_size, os_info_file);
		}

		/* Free the allocated line buffer */
		if (line_buf != NULL)
		{
			free(line_buf);
			line_buf = NULL;
		}

		fclose(os_info_file);
	}

	/* Get total file descriptor, thread count and process count */
	if (read_process_status(&active_processes, &running_processes, &sleeping_processes,
							&stopped_processes, &zombie_processes, &total_threads))
	{
		values[Anum_os_process_count] = active_processes;
		values[Anum_os_thread_count] = total_threads;
	}
	else
	{
		nulls[Anum_os_process_count] = true;
		nulls[Anum_os_thread_count] = true;
	}

	/* licenced user is not applicable to linux so return NULL */
	nulls[Anum_os_boot_time] = true;

	/* count the total number of opended file descriptor */
	if (!total_opened_handle(&handle_count))
		nulls[Anum_os_handle_count] = true;

	if (sysinfo(&s_info) != 0)
		nulls[Anum_os_up_since_seconds] = true;
	else
		values[Anum_os_up_since_seconds] = Int32GetDatum((int)s_info.uptime);

	values[Anum_os_name]             = CStringGetTextDatum(os_name);
	values[Anum_os_version]          = CStringGetTextDatum(version);
	values[Anum_host_name]           = CStringGetTextDatum(host_name);
	values[Anum_domain_name]         = CStringGetTextDatum(domain_name);
	values[Anum_os_handle_count]     = Int32GetDatum(handle_count);
	values[Anum_os_architecture]     = CStringGetTextDatum(architecture);

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);
}
