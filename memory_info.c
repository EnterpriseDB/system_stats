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

void ReadMemoryInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	FILE       *memory_file;
	Datum      values[Natts_memory_info];
	bool       nulls[Natts_memory_info];
	char       *line_buf = NULL;
	size_t     line_buf_size = 0;
	int        line_count = 0;
	ssize_t    line_size;
	uint64     total_memory = 0;
	uint64     free_memory = 0;
	uint64     available_memory = 0;
	uint64     buffers = 0;
	uint64     cached = 0;
	uint64     swap_cached = 0;
	uint64     swap_total = 0;
	uint64     swap_free = 0;

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
		if (strstr(line_buf, "MemTotal") != NULL)
		{
			line_count++;
			total_memory = ConvertToBytes(line_buf);
		}

		/* Read the free memory of the system */
		if (strstr(line_buf, "MemFree") != NULL)
		{
			line_count++;
			free_memory = ConvertToBytes(line_buf);
		}

		/* Read the available memory of the system */
		if (strstr(line_buf, "MemAvailable") != NULL)
		{
			line_count++;
			available_memory = ConvertToBytes(line_buf);
		}

		/* Read the buffer memory of the system */
		if (strstr(line_buf, "Buffers") != NULL)
		{
			line_count++;
			buffers = ConvertToBytes(line_buf);
		}

		/* Read the cached memory of the system */
		if (strstr(line_buf, "Cached") != NULL)
		{
			line_count++;
			cached = ConvertToBytes(line_buf);
		}

		/* Read the cached swap memory of the system */
		if (strstr(line_buf, "SwapCached") != NULL)
		{
			line_count++;
			swap_cached = ConvertToBytes(line_buf);
		}

		/* Read the total swap memory of the system */
		if (strstr(line_buf, "SwapTotal") != NULL)
		{
			line_count++;
			swap_total = ConvertToBytes(line_buf);
		}

		/* Read the free swap memory of the system */
		if (strstr(line_buf, "SwapFree") != NULL)
		{
			line_count++;
			swap_free = ConvertToBytes(line_buf);
		}

		// Check if we get all lines, add as row
		if (line_count == Natts_memory_info)
		{
			values[Anum_total_memory] = Int64GetDatumFast(total_memory);
			values[Anum_free_memory] = Int64GetDatumFast(free_memory);
			values[Anum_available_memory] = Int64GetDatumFast(available_memory);
			values[Anum_buffers] = Int64GetDatumFast(buffers);
			values[Anum_cached] = Int64GetDatumFast(cached);
			values[Anum_swap_cached] = Int64GetDatumFast(swap_cached);
			values[Anum_swap_total] = Int64GetDatumFast(swap_total);
			values[Anum_swap_free] = Int64GetDatumFast(swap_free);

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
