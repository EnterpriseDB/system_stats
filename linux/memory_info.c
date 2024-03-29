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

void ReadMemoryInformation(Tuplestorestate *tupstore, TupleDesc tupdesc);

void ReadMemoryInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	FILE       *memory_file;
	Datum      values[Natts_memory_info];
	bool       nulls[Natts_memory_info];
	char       *line_buf = NULL;
	size_t     line_buf_size = 0;
	int        line_count = 0;
	ssize_t    line_size;
	uint64     total_memory_bytes = 0;
	uint64     free_memory_bytes = 0;
	uint64     used_memory_bytes = 0;
	uint64     cached_bytes = 0;
	uint64     swap_total_bytes = 0;
	uint64     swap_free_bytes = 0;
	uint64     swap_used_bytes = 0;

	memset(nulls, 0, sizeof(nulls));

	/* Open the file required to read all the memory information */
	memory_file = fopen(MEMORY_FILE_NAME, "r");

	if (!memory_file)
	{
		char memory_file_name[MAXPGPATH];
		snprintf(memory_file_name, MAXPGPATH, "%s", MEMORY_FILE_NAME);

		ereport(DEBUG1,
				(errcode_for_file_access(),
					errmsg("can not open file %s for reading memory information",
						memory_file_name)));
		return;
	}

	/* Get the first line of the file. */
	line_size = getline(&line_buf, &line_buf_size, memory_file);

	/* Loop through until we are done with the file. */
	while (line_size >= 0)
	{
		/* Read the total memory of the system */
		char *mem_total = strstr(line_buf, "MemTotal:");
		char *mem_free = NULL;
		char *cached_mem = NULL;
		char *c_swap_total = NULL;
		char *c_swap_free = NULL;
		if (mem_total != NULL && mem_total == line_buf)
		{
			line_count++;
			total_memory_bytes = ConvertToBytes(line_buf);
		}

		/* Read the free memory of the system */
		mem_free = strstr(line_buf, "MemFree:");
		if (mem_free != NULL && mem_free == line_buf)
		{
			line_count++;
			free_memory_bytes = ConvertToBytes(line_buf);
		}

		/* Read the cached memory of the system */
		cached_mem = strstr(line_buf, "Cached:");
		if (cached_mem != NULL && cached_mem == line_buf)
		{
			line_count++;
			cached_bytes = ConvertToBytes(line_buf);
		}

		/* Read the total swap memory of the system */
		c_swap_total = strstr(line_buf, "SwapTotal:");
		if (c_swap_total != NULL && c_swap_total == line_buf)
		{
			line_count++;
			swap_total_bytes = ConvertToBytes(line_buf);
		}

		/* Read the free swap memory of the system */
		c_swap_free = strstr(line_buf, "SwapFree:");
		if (c_swap_free != NULL && c_swap_free == line_buf)
		{
			line_count++;
			swap_free_bytes = ConvertToBytes(line_buf);
		}

		used_memory_bytes = total_memory_bytes - free_memory_bytes;
		swap_used_bytes = swap_total_bytes - swap_free_bytes;

		// Check if we get all lines, add as row
		if (line_count == MEMORY_READ_COUNT)
		{
			values[Anum_total_memory] = UInt64GetDatum(total_memory_bytes);
			values[Anum_free_memory] = UInt64GetDatum(free_memory_bytes);
			values[Anum_used_memory] = UInt64GetDatum(used_memory_bytes);
			values[Anum_total_cache_memory] = UInt64GetDatum(cached_bytes);
			values[Anum_swap_total_memory] = UInt64GetDatum(swap_total_bytes);
			values[Anum_swap_free_memory] = UInt64GetDatum(swap_free_bytes);
			values[Anum_swap_used_memory] = UInt64GetDatum(swap_used_bytes);

			/* set the NULL value as it is not for this platform */
			nulls[Anum_kernel_total_memory] = true;
			nulls[Anum_kernel_paged_memory] = true;
			nulls[Anum_kernel_nonpaged_memory] = true;
			nulls[Anum_total_page_file] = true;
			nulls[Anum_avail_page_file] = true;

			tuplestore_putvalues(tupstore, tupdesc, values, nulls);

			/* Free the allocated line buffer */
			if (line_buf != NULL)
			{
				free(line_buf);
				line_buf = NULL;
			}

			break;
		}

		/* Free the allocated line buffer */
		if (line_buf != NULL)
		{
			free(line_buf);
			line_buf = NULL;
		}

		/* Get the next line */
		line_size = getline(&line_buf, &line_buf_size, memory_file);
	}

	/* Free the allocated line buffer */
	if (line_buf != NULL)
	{
		free(line_buf);
		line_buf = NULL;
	}

	/* Close the file now that we are done with it */
	fclose(memory_file);
}
