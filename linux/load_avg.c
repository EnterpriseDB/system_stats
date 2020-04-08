/*------------------------------------------------------------------------
 * load_avg.c
 *              System load average information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "stats.h"

void ReadLoadAvgInformations(Tuplestorestate *tupstore, TupleDesc tupdesc);

void ReadLoadAvgInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	FILE       *loadavg_file;
	char       *line_buf = NULL;
	size_t     line_buf_size = 0;
	ssize_t    line_size;
	Datum      values[Natts_load_avg_info];
	bool       nulls[Natts_load_avg_info];
	float4     load_avg_one_minute = 0;
	float4     load_avg_five_minutes = 0;
	float4     load_avg_ten_minutes = 0;
	const char *scan_fmt = "%f %f %f";

	memset(nulls, 0, sizeof(nulls));

	loadavg_file = fopen(CPU_IO_LOAD_AVG_FILE, "r");

	if (!loadavg_file)
	{
		char loadavg_file_name[MAXPGPATH];
		snprintf(loadavg_file_name, MAXPGPATH, "%s", CPU_IO_LOAD_AVG_FILE);

		ereport(DEBUG1,
				(errcode_for_file_access(),
					errmsg("can not open file %s for reading load avg information",
						loadavg_file_name)));
		return;
	}

	/* Get the first line of the file. */
	line_size = getline(&line_buf, &line_buf_size, loadavg_file);

	/* Loop through until we are done with the file. */
	if (line_size >= 0)
	{
		sscanf(line_buf, scan_fmt, &load_avg_one_minute, &load_avg_five_minutes, &load_avg_ten_minutes);

		values[Anum_load_avg_one_minute]   = Float4GetDatum(load_avg_one_minute);
		values[Anum_load_avg_five_minutes] = Float4GetDatum(load_avg_five_minutes);
		values[Anum_load_avg_ten_minutes]  = Float4GetDatum(load_avg_ten_minutes);

		nulls[Anum_load_avg_fifteen_minutes] = true;

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);

		//reset the value again
		load_avg_one_minute = 0;
		load_avg_five_minutes = 0;
		load_avg_ten_minutes = 0;
	}

	if (line_buf != NULL)
	{
		free(line_buf);
		line_buf = NULL;
	}

	fclose(loadavg_file);
}
