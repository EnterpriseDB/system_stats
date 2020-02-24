/*------------------------------------------------------------------------
 * process_info.c
 *              System process information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"

#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

void ReadProcessInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	FILE          *fpstat;
	DIR           *dirp;
	struct dirent *ent, dbuf;
	int           active_processes = 0;
	int           running_processes = 0;
	int           sleeping_processes = 0;
	int           stopped_processes = 0;
	int           zombie_processes = 0;
	char          file_name[MIN_BUFFER_SIZE];
	char          process_type;
	Datum         values[Natts_process_info];
	bool          nulls[Natts_process_info];

	memset(nulls, 0, sizeof(nulls));

	dirp = opendir(PROC_FILE_SYSTEM_PATH);
	if (!dirp)
	{
		ereport(DEBUG1, (errmsg("Error opening /proc directory")));
		return;
	}

	/* Read the proc directory for process status */
	while (readdir_r(dirp, &dbuf, &ent) == 0)
	{
		memset(file_name, 0x00, MIN_BUFFER_SIZE);
		process_type = '\0';

		if (!ent)
			break;

		/* Iterate only digit as name because it is process id */
		if (!isdigit(*ent->d_name))
			continue;

		active_processes++;

		sprintf(file_name,"/proc/%s/stat", ent->d_name);

		fpstat = fopen(file_name, "r");
		if (fpstat == NULL)
			continue;

		if (fscanf(fpstat, "%*d %*s %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u"
					"%*d %*d %*d %*d %*d %*d %*u %*u %*d", &process_type) == EOF)
			ereport(DEBUG1, (errmsg("Error in parsing file '%s'", file_name)));

		if (process_type == 'R')
			running_processes++;
		else if(process_type == 'S' || process_type == 'D')
			sleeping_processes++;
		else if (process_type == 'T')
			stopped_processes++;
		else if (process_type == 'Z')
			zombie_processes++;
		else
			ereport(DEBUG1, (errmsg("Invalid process type '%c'", process_type)));

		fclose(fpstat);
		fpstat = NULL;
	}

	closedir(dirp);
	dirp = NULL;

	values[Anum_active_processes] = Int32GetDatum(active_processes);
	values[Anum_running_processes] = Int32GetDatum(running_processes);
	values[Anum_sleeping_processes] = Int32GetDatum(sleeping_processes);
	values[Anum_stopped_processes] = Int32GetDatum(stopped_processes);
	values[Anum_zombie_processes] = Int32GetDatum(zombie_processes);

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);
}
