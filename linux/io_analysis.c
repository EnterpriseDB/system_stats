/*------------------------------------------------------------------------
 * io_analysis.c
 *              System IO analysis information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"

void ReadIOAnalysisInformation(Tuplestorestate *tupstore, TupleDesc tupdesc);

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
	char       file_name[MAXPGPATH];
	uint64     read_completed = 0;
	uint64     sector_read = 0;
	uint64     time_spent_reading_ms = 0;
	uint64     write_completed = 0;
	uint64     sector_written = 0;
	uint64     time_spent_writing_ms = 0;
	uint64     sector_size = 512;
	const char *scan_fmt = "%*d %*d %s %lld %*lld %lld %lld %lld %*lld %lld %lld";

	memset(nulls, 0, sizeof(nulls));
	memset(device_name, 0, MAXPGPATH);
	memset(file_name, 0, MAXPGPATH);

	/* file name used to read the sector size in bytes */
	sprintf(file_name, "/sys/block/sda/queue/hw_sector_size");
	ReadFileContent(file_name, &sector_size);

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
	if (line_size >= 0)
	{
		sscanf(line_buf, scan_fmt, device_name, &read_completed, &sector_read, &time_spent_reading_ms,
		  &write_completed, &sector_written, &time_spent_writing_ms);

		values[Anum_device_name] = CStringGetTextDatum(device_name);
		values[Anum_total_read] = Int64GetDatumFast(read_completed);
		values[Anum_total_write] = Int64GetDatumFast(write_completed);
		values[Anum_read_bytes] = Int64GetDatumFast((uint64)(sector_read * sector_size));
		values[Anum_write_bytes] = Int64GetDatumFast((uint64)(sector_written * sector_size));
		values[Anum_read_time_ms] = Int64GetDatumFast((uint64)time_spent_reading_ms);
		values[Anum_write_time_ms] = Int64GetDatumFast((uint64)time_spent_writing_ms);

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);
	}

	if (line_buf != NULL)
	{
		free(line_buf);
		line_buf = NULL;
	}

	fclose(diskstats_file);
}
