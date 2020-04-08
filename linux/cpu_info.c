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
#include <sys/utsname.h>

#define L1D_CACHE_FILE_PATH  "/sys/devices/system/cpu/cpu0/cache/index0/size"
#define L1I_CACHE_FILE_PATH  "/sys/devices/system/cpu/cpu0/cache/index1/size"
#define L2_CACHE_FILE_PATH   "/sys/devices/system/cpu/cpu0/cache/index2/size"
#define L3_CACHE_FILE_PATH   "/sys/devices/system/cpu/cpu0/cache/index3/size"

int read_cpu_cache_size(const char *path);
void ReadCPUInformation(Tuplestorestate *tupstore, TupleDesc tupdesc);

int read_cpu_cache_size(const char *path)
{
	FILE          *fp;
	char          *line_buf = NULL;
	size_t        line_buf_size = 0;
	ssize_t       line_size;
	int           cache_size = 0;

	fp = fopen(L1D_CACHE_FILE_PATH, "r");
	if (!fp)
	{
		ereport(DEBUG1, (errmsg("can not open file{%s) for reading", L1D_CACHE_FILE_PATH)));
		cache_size = 0;
	}
	else
	{
		/* Get the first line of the file. */
		line_size = getline(&line_buf, &line_buf_size, fp);

		/* Loop through until we are done with the file. */
		if (line_size >= 0)
		{
			int len;
			size_t index;
			len = strlen(line_buf);
			for(index = 0; index < len; index++)
			{
				if( !isdigit(line_buf[index]))
				{
					line_buf[index] = '\0';
					break;
				}
			}
		}

		cache_size = atoi(line_buf);

		if (line_buf != NULL)
		{
			free(line_buf);
			line_buf = NULL;
		}

		fclose(fp);
	}

	return cache_size;
}

void ReadCPUInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	struct     utsname uts;
	char       *found;
	FILE       *cpu_info_file;
	Datum      values[Natts_cpu_info];
	bool       nulls[Natts_cpu_info];
	char       vendor_id[MAXPGPATH];
	char       cpu_family[MAXPGPATH];
	char       cpu_desc[MAXPGPATH];
	char       model[MAXPGPATH];
	char       model_name[MAXPGPATH];
	char       cpu_mhz[MAXPGPATH];
	char       architecture[MAXPGPATH];
	char       *line_buf = NULL;
	size_t     line_buf_size = 0;
	ssize_t    line_size;
	bool       model_found = false;
	int        ret_val;
	int        physical_processor = 0;
	int        logical_processor = 0;
	int        l1dcache_size = 0;
	int        l1icache_size = 0;
	int        l2cache_size = 0;
	int        l3cache_size = 0;
	int        cpu_cores = 0;

	memset(nulls, 0, sizeof(nulls));
	memset(vendor_id, 0, MAXPGPATH);
	memset(cpu_family, 0, MAXPGPATH);
	memset(model, 0, MAXPGPATH);
	memset(model_name, 0, MAXPGPATH);
	memset(cpu_mhz, 0, MAXPGPATH);
	memset(cpu_desc, 0, MAXPGPATH);

	l1dcache_size = read_cpu_cache_size(L1D_CACHE_FILE_PATH);
	l1icache_size = read_cpu_cache_size(L1I_CACHE_FILE_PATH);
	l2cache_size = read_cpu_cache_size(L2_CACHE_FILE_PATH);
	l3cache_size = read_cpu_cache_size(L3_CACHE_FILE_PATH);

	ret_val = uname(&uts);
	/* if it returns not zero means it fails so set null values */
	if (ret_val != 0)
		nulls[Anum_architecture] = true;
	else
		memcpy(architecture, uts.machine, strlen(uts.machine));

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
						{
							physical_processor++;
							memcpy(cpu_mhz, found, strlen(found));
						}
						if (strstr(line_buf, "cpu cores") != NULL)
							cpu_cores = atoi(found);
					}
				}

				/* Free the allocated line buffer */
				if (line_buf != NULL)
				{
					free(line_buf);
					line_buf = NULL;
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

		if (physical_processor)
		{
			sprintf(cpu_desc, "%s model %s family %s", vendor_id, model, cpu_family);
			values[Anum_cpu_vendor] = CStringGetTextDatum(vendor_id);
			values[Anum_cpu_description] = CStringGetTextDatum(cpu_desc);
			values[Anum_model_name] = CStringGetTextDatum(model_name);
			values[Anum_logical_processor] = Int32GetDatum(logical_processor);
			values[Anum_physical_processor] = Int32GetDatum(physical_processor);
			values[Anum_no_of_cores] = Int32GetDatum(cpu_cores);
			values[Anum_architecture] = CStringGetTextDatum(architecture);
			values[Anum_cpu_clock_speed] = CStringGetTextDatum(cpu_mhz);
			values[Anum_l1dcache_size] = Int32GetDatum(l1dcache_size);
			values[Anum_l1icache_size] = Int32GetDatum(l1icache_size);
			values[Anum_l2cache_size] = Int32GetDatum(l2cache_size);
			values[Anum_l3cache_size] = Int32GetDatum(l3cache_size);

			nulls[Anum_processor_type] = true;

			tuplestore_putvalues(tupstore, tupdesc, values, nulls);
		}
	}
}
