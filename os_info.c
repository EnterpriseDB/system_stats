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

#include <unistd.h>
#include <sys/utsname.h>

void ReadOSInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	struct     utsname uts;
	Datum      values[Natts_os_info];
	bool       nulls[Natts_os_info];
	char       host_name[MAXPGPATH];
	char       domain_name[MAXPGPATH];
	char       kernel_info[MAXPGPATH];
	char       architecture[MAXPGPATH];
	char       os_description[MAXPGPATH];
	char       os_release_version[MAXPGPATH];
	char       os_codename[MAXPGPATH];
	int        ret_val;
	FILE       *os_info_file;
	char       *line_buf = NULL;
	size_t     line_buf_size = 0;
	ssize_t    line_size;

	memset(nulls, 0, sizeof(nulls));
	memset(host_name, 0, MAXPGPATH);
	memset(domain_name, 0, MAXPGPATH);
	memset(kernel_info, 0, MAXPGPATH);
	memset(architecture, 0, MAXPGPATH);
	memset(os_description, 0, MAXPGPATH);
	memset(os_release_version, 0, MAXPGPATH);
	memset(os_codename, 0, MAXPGPATH);

	ret_val = uname(&uts);
	/* if it returns not zero means it fails so set null values */
	if (ret_val != 0)
	{
		nulls[Anum_kernel_info]  = true;
		nulls[Anum_architecture] = true;
	}
	else
	{
		sprintf(kernel_info, "%s %s", uts.sysname, uts.release);
		memcpy(architecture, uts.machine, strlen(uts.machine));
	}

	/* Function used to get the host name of the system */
	if (gethostname(host_name, sizeof(host_name)) != 0)
		ereport(DEBUG1,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("error while getting host name")));

	/* Function used to get the domain name of the system */
	if (getdomainname(domain_name, sizeof(domain_name)) != 0)
		ereport(DEBUG1,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("error while getting domain name")));

	/*If hostname or domain name is empty, set the value to NULL */
	if (strlen(host_name) == 0)
		nulls[Anum_host_name] = true;
	if (strlen(domain_name) == 0)
		nulls[Anum_domain_name] = true;

	os_info_file = fopen(OS_INFO_FILE_NAME, "r");

	if (!os_info_file)
	{
		char os_info_file_name[MAXPGPATH];
		snprintf(os_info_file_name, MAXPGPATH, "%s", OS_INFO_FILE_NAME);

		ereport(DEBUG1,
				(errcode_for_file_access(),
					errmsg("can not open file %s for reading os information",
						os_info_file_name)));

		nulls[Anum_os_description] = true;
		nulls[Anum_os_release_version] = true;
		nulls[Anum_os_codename] = true;
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
				memcpy(os_description, (line_buf + strlen(OS_DESC_SEARCH_TEXT)), (len - strlen(OS_DESC_SEARCH_TEXT)));
			if (strstr(line_buf, OS_VERSION_SEARCH_TEXT) != NULL)
				memcpy(os_release_version, (line_buf + strlen(OS_VERSION_SEARCH_TEXT)), (len - strlen(OS_VERSION_SEARCH_TEXT)));
			if (strstr(line_buf, OS_CODE_NAME_SEARCH_TEXT) != NULL)
				memcpy(os_codename, (line_buf + strlen(OS_CODE_NAME_SEARCH_TEXT)), (len - strlen(OS_CODE_NAME_SEARCH_TEXT)));

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

	values[Anum_host_name]           = CStringGetTextDatum(host_name);
	values[Anum_domain_name]         = CStringGetTextDatum(domain_name);
	values[Anum_kernel_info]         = CStringGetTextDatum(kernel_info);
	values[Anum_architecture]        = CStringGetTextDatum(architecture);
	values[Anum_os_description]      = CStringGetTextDatum(os_description);
	values[Anum_os_release_version]  = CStringGetTextDatum(os_release_version);
	values[Anum_os_codename]         = CStringGetTextDatum(os_codename);

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);
}
