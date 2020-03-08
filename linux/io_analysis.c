/*------------------------------------------------------------------------
 * io_analysis.c
 *              System IO analysis information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "stats.h"

/* Function used to get IO statistics of block devices */
void ReadIOAnalysisInformation(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	FILE       *diskstats_file;
	Datum      values[Natts_io_analysis_info];
	bool       nulls[Natts_io_analysis_info];
	char       *line_buf = NULL;
	size_t     line_buf_size = 0;
	ssize_t    line_size;
	char       device_name[MAXPGPATH];
	uint64     major_no = 0;
	uint64     minor_no = 0;
	uint64     read_completed = 0;
	uint64     read_merged = 0;
	uint64     sector_read = 0;
	uint64     time_spent_reading_ms = 0;
	uint64     write_completed = 0;
	uint64     write_merged = 0;
	uint64     sector_written = 0;
	uint64     time_spent_writing_ms = 0;
	uint64     io_in_progress = 0;
	uint64     time_spent_io_ms = 0;
	uint64     weighted_time_spent_io_ms = 0;

	memset(nulls, 0, sizeof(nulls));
	memset(device_name, 0, MAXPGPATH);

	diskstats_file = fopen(DISK_IO_STATS_FILE_NAME, "r");

	if (!diskstats_file)
	{
		char disk_file_name[MAXPGPATH];
		snprintf(disk_file_name, MAXPGPATH, "%s", DISK_IO_STATS_FILE_NAME);

		ereport(DEBUG1,
				(errcode_for_file_access(),
					errmsg("can not open file %s for reading disk stats information",
						disk_file_name)));
		return;
	}

	/* Get the first line of the file. */
	line_size = getline(&line_buf, &line_buf_size, diskstats_file);

	/* Loop through until we are done with the file. */
	while (line_size >= 0)
	{
		char result[MAXPGPATH];
		char *token = NULL;
		int index = 0;

		memset(result, 0, MAXPGPATH);

		token = strtok(line_buf, " ");
		/* Parse the file output and read the required information for io */
		while(token != NULL)
		{
			if (index == Anum_major_no)
				major_no = (uint64_t)atoll(trimStr(token));
			if (index == Anum_minor_no)
				minor_no = (uint64_t)atoll(trimStr(token));
			if (index == Anum_device_name)
				memcpy(device_name, token, strlen(token));
			if (index == Anum_read_completed)
				read_completed = (uint64_t)atoll(trimStr(token));
			if (index == Anum_read_merged)
				read_merged = (uint64_t)atoll(trimStr(token));
			if (index == Anum_sector_read)
				sector_read = (uint64_t)atoll(trimStr(token));
			if (index == Anum_time_spent_reading_ms)
				time_spent_reading_ms = (uint64_t)atoll(trimStr(token));
			if (index == Anum_write_completed)
				write_completed = (uint64_t)atoll(trimStr(token));
			if (index == Anum_write_merged)
				write_merged = (uint64_t)atoll(trimStr(token));
			if (index == Anum_sector_written)
				sector_written = (uint64_t)atoll(trimStr(token));
			if (index == Anum_time_spent_writing_ms)
				time_spent_writing_ms = (uint64_t)atoll(trimStr(token));
			if (index == Anum_io_in_progress)
				io_in_progress = (uint64_t)atoll(trimStr(token));
			if (index == Anum_time_spent_io_ms)
				time_spent_io_ms = (uint64_t)atoll(trimStr(token));
			if (index == Anum_weighted_time_spent_io_ms)
				weighted_time_spent_io_ms = (uint64_t)atoll(trimStr(token));

			token = strtok(NULL, " ");
			index++;
		}

		values[Anum_major_no] = Int64GetDatumFast(major_no);
		values[Anum_minor_no] = Int64GetDatumFast(minor_no);
		values[Anum_device_name] = CStringGetTextDatum(device_name);
		values[Anum_read_completed] = Int64GetDatumFast(read_completed);
		values[Anum_read_merged] = Int64GetDatumFast(read_merged);
		values[Anum_sector_read] = Int64GetDatumFast(sector_read);
		values[Anum_time_spent_reading_ms] = Int64GetDatumFast(time_spent_reading_ms);
		values[Anum_write_completed] = Int64GetDatumFast(write_completed);
		values[Anum_write_merged] = Int64GetDatumFast(write_merged);
		values[Anum_sector_written] = Int64GetDatumFast(sector_written);
		values[Anum_time_spent_writing_ms] = Int64GetDatumFast(time_spent_writing_ms);
		values[Anum_io_in_progress] = Int64GetDatumFast(io_in_progress);
		values[Anum_time_spent_io_ms] = Int64GetDatumFast(time_spent_io_ms);
		values[Anum_weighted_time_spent_io_ms] = Int64GetDatumFast(weighted_time_spent_io_ms);

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);

		/* Free the allocated line buffer */
		if (line_buf != NULL)
		{
			free(line_buf);
			line_buf = NULL;
		}

		/* Get the next line */
		line_size = getline(&line_buf, &line_buf_size, diskstats_file);
	}

	/* Free the allocated line buffer */
	if (line_buf != NULL)
	{
		free(line_buf);
		line_buf = NULL;
	}

	/* Close the file now that we are done with it */
	fclose(diskstats_file);
}
