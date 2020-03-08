/*------------------------------------------------------------------------
 * cpu_info.c
 *              System CPU information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "stats.h"

void ReadCPUInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	char       *found;
	FILE       *cpu_info_file;
	Datum      values[Natts_cpu_info];
	bool       nulls[Natts_cpu_info];
	int        pro_val = 0;
	char       vendor_id[MAXPGPATH];
	char       cpu_family[MAXPGPATH];
	char       model[MAXPGPATH];
	char       model_name[MAXPGPATH];
	char       cpu_mhz[MAXPGPATH];
	char       cpu_cache_size[MAXPGPATH];
	char       *line_buf = NULL;
	size_t     line_buf_size = 0;
	ssize_t    line_size;
	bool       model_found = false;

	memset(nulls, 0, sizeof(nulls));
	memset(vendor_id, 0, MAXPGPATH);
	memset(cpu_family, 0, MAXPGPATH);
	memset(model, 0, MAXPGPATH);
	memset(model_name, 0, MAXPGPATH);
	memset(cpu_mhz, 0, MAXPGPATH);
	memset(cpu_cache_size, 0, MAXPGPATH);

	cpu_info_file = fopen(CPU_INFO_FILE_NAME, "r");

	if (!cpu_info_file)
	{
		char cpu_info_file_name[MAXPGPATH];
		snprintf(cpu_info_file_name, MAXPGPATH, "%s", CPU_INFO_FILE_NAME);

		ereport(DEBUG1,
				(errcode_for_file_access(),
				errmsg("can not open file %s for reading cpu information",
					cpu_info_file_name)));
		return;
	}
	else
	{
		/* Get the first line of the file. */
		line_size = getline(&line_buf, &line_buf_size, cpu_info_file);

		/* Loop through until we are done with the file. */
		while (line_size >= 0)
		{
			if (strlen(line_buf) > 0)
				line_buf = trimStr(line_buf);

			if (!IS_EMPTY_STR(line_buf) && (strlen(line_buf) > 0))
			{
				if (strlen(line_buf) > 0)
				{
					found = strstr(line_buf, ":");
					if (strlen(found) > 0)
					{
						found = trimStr((found+1));

						if (strstr(line_buf, "processor") != NULL)
							pro_val = atoi(found);
						if (strstr(line_buf, "vendor_id") != NULL)
							memcpy(vendor_id, found, strlen(found));
						if (strstr(line_buf, "cpu family") != NULL)
							memcpy(cpu_family, found, strlen(found));
						if (strstr(line_buf, "model") != NULL && !model_found)
						{
							memcpy(model, found, strlen(found));
							model_found = true;
						}
						if (strstr(line_buf, "model name") != NULL)
							memcpy(model_name, found, strlen(found));
						if (strstr(line_buf, "cpu MHz") != NULL)
							memcpy(cpu_mhz, found, strlen(found));
						if (strstr(line_buf, "cache size") != NULL)
							memcpy(cpu_cache_size, found, strlen(found));
					}
				}

				/* Free the allocated line buffer */
				if (line_buf != NULL)
				{
					free(line_buf);
					line_buf = NULL;
				}
			}
			else
			{
				if (strlen(cpu_mhz) > 0)
				{
					values[Anum_processor] = Int32GetDatum(pro_val);
					values[Anum_vendor_id] = CStringGetTextDatum(vendor_id);
					values[Anum_cpu_family] = CStringGetTextDatum(cpu_family);
					values[Anum_model] = CStringGetTextDatum(model);
					values[Anum_model_name] = CStringGetTextDatum(model_name);
					values[Anum_cpu_mhz] = CStringGetTextDatum(cpu_mhz);
					values[Anum_cpu_cache_size] = CStringGetTextDatum(cpu_cache_size);

					tuplestore_putvalues(tupstore, tupdesc, values, nulls);

					/* Reset the value again */
					pro_val = 0;
					memset(vendor_id, 0, MAXPGPATH);
					memset(cpu_family, 0, MAXPGPATH);
					memset(model, 0, MAXPGPATH);
					memset(model_name, 0, MAXPGPATH);
					memset(cpu_mhz, 0, MAXPGPATH);
					memset(cpu_cache_size, 0, MAXPGPATH);
					model_found = false;
				}
			}

			/* Get the next line */
			line_size = getline(&line_buf, &line_buf_size, cpu_info_file);
		}

		/* Free the allocated line buffer */
		if (line_buf != NULL)
		{
			free(line_buf);
			line_buf = NULL;
		}

		fclose(cpu_info_file);
	}
}
