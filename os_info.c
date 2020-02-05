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

void ReadOSInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	char       *output;
	char       *new_line_token;
	Datum      values[Natts_os_info];
	bool       nulls[Natts_os_info];
	char       host_name[MAXPGPATH];
	char       domain_name[MAXPGPATH];
	char       kernel_info[MAXPGPATH];
	char       architecture[MAXPGPATH];
	char       os_distribution_id[MAXPGPATH];
	char       os_description[MAXPGPATH];
	char       os_release_version[MAXPGPATH];
	char       os_codename[MAXPGPATH];

	memset(nulls, 0, sizeof(nulls));
	memset(host_name, 0, MAXPGPATH);
	memset(domain_name, 0, MAXPGPATH);
	memset(kernel_info, 0, MAXPGPATH);
	memset(architecture, 0, MAXPGPATH);
	memset(os_distribution_id, 0, MAXPGPATH);
	memset(os_description, 0, MAXPGPATH);
	memset(os_release_version, 0, MAXPGPATH);
	memset(os_codename, 0, MAXPGPATH);

	/* Run the command and read the output */
	output = runCommand(OS_INFO_COMMAND_1);

	if (!output)
		return;
	else
	{
		/* Function used to get the host name of the system */
		if (gethostname(host_name, sizeof(host_name)) != 0)
			ereport(WARNING,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("error while getting host name")));

		/* Function used to get the domain name of the system */
		if (getdomainname(domain_name, sizeof(domain_name)) != 0)
			ereport(WARNING,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("error while getting domain name")));

		/*If hostname or domain name is empty, set the value to NULL */
		if (strlen(host_name) == 0)
			nulls[Anum_host_name] = true;
		if (strlen(domain_name) == 0)
			nulls[Anum_domain_name] = true;

		/* Parse the command output and find kernel version and architecture */
		for (new_line_token = strtok(output,"\n"); new_line_token != NULL; new_line_token = strtok(NULL, "\n"))
		{
			char *found = strstr(new_line_token, ":");
			found = trimStr((found+1));

			if (strstr(new_line_token, "Kernel") != NULL)
				memcpy(kernel_info, found, strlen(found));

			if (strstr(new_line_token, "Architecture") != NULL)
				memcpy(architecture, found, strlen(found));
		}
	}

	/* Free the allocated buffer) */
	if (output != NULL)
	{
		pfree(output);
		output = NULL;
	}

	/* Run the command and read the output */
	output = runCommand(OS_INFO_COMMAND_2);

	if (output != NULL)
	{
		/* Parse the command output and find operating system information */
		for (new_line_token = strtok(output,"\n"); new_line_token != NULL; new_line_token = strtok(NULL, "\n"))
		{
			char *found = strstr(new_line_token, ":");
			found = trimStr((found+1));

			if (strstr(new_line_token, "Distributor ID") != NULL)
				memcpy(os_distribution_id, found, strlen(found));

			if (strstr(new_line_token, "Description") != NULL)
				memcpy(os_description, found, strlen(found));

			if (strstr(new_line_token, "Release") != NULL)
				memcpy(os_release_version, found, strlen(found));

			if (strstr(new_line_token, "Codename") != NULL)
				memcpy(os_codename, found, strlen(found));
		}
	}

	/* Free the allocated buffer) */
	if (output != NULL)
	{
		pfree(output);
		output = NULL;
	}

	/* if we didn't get os release version, check again for another command */
	if (IS_EMPTY_STR(os_description))
	{
		/* Run the command and read the output */
		output = runCommand(OS_INFO_COMMAND_3);

		if (output != NULL)
		{
			bool os_desc_found = false;
			bool os_version_found = false;
			bool os_desc_id_found = false;
			/* Parse the command output and find operating system information */
			for (new_line_token = strtok(output,"\n"); new_line_token != NULL; new_line_token = strtok(NULL, "\n"))
			{
				char *found = strstr(new_line_token, "=");
				// As it didn't found the token, means the whole line will be name of the OS
				if (found == NULL)
				{
					if (!os_desc_found)
					{
						os_desc_found = true;
						memcpy(os_description, new_line_token, strlen(new_line_token));
					}
				}
				else
				{
					found = trimStr((found+1));
					if (strstr(new_line_token, "PRETTY_NAME") != NULL)
						memcpy(os_description, found, strlen(found));
					if (strstr(new_line_token, "VERSION=") != NULL && !os_version_found)
					{
						os_version_found = true;
						memcpy(os_release_version, found, strlen(found));
					}
					if (strstr(new_line_token, "ID=") != NULL && !os_desc_id_found)
					{
						os_desc_id_found = true;
						memcpy(os_distribution_id, found, strlen(found));
					}
				}
			}
		}

		/* Free the allocated buffer) */
		if (output != NULL)
		{
			pfree(output);
			output = NULL;
		}
	}

	values[Anum_host_name]           = CStringGetTextDatum(host_name);
	values[Anum_domain_name]         = CStringGetTextDatum(domain_name);
	values[Anum_kernel_info]         = CStringGetTextDatum(kernel_info);
	values[Anum_architecture]        = CStringGetTextDatum(architecture);
	values[Anum_os_distribution_id]  = CStringGetTextDatum(os_distribution_id);
	values[Anum_os_description]      = CStringGetTextDatum(os_description);
	values[Anum_os_release_version]  = CStringGetTextDatum(os_release_version);
	values[Anum_os_codename]         = CStringGetTextDatum(os_codename);

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);
}
