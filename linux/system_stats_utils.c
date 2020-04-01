/*------------------------------------------------------------------------
 * system_stats_utils.c
 *              Defined required utility functions to fetch
 *              system statistics information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */
#include "postgres.h"
#include "stats.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <dirent.h>

char* leftTrimStr(char* s);
char* rightTrimStr(char* s);

/* Function used to convert KB, MB, GB to bytes */
uint64_t ConvertToBytes(char *line_buf)
{
	uint64_t value = 0;
	char *found = strstr(line_buf, ":");
	if (found)
	{
		char result[MAXPGPATH];
		char suffix[MAXPGPATH];
		char *token = NULL;
		int icount = 0;
		memset(result, 0x00, MAXPGPATH);
		memset(suffix, 0x00, MAXPGPATH);
		found = trimStr((found+1));

		token = strtok(found, " ");
		while( token != NULL )
		{
			if (icount == 0)
				memcpy(result, token, strlen(token));
			else
			{
				memcpy(suffix, token, strlen(token));
				break;
			}

			token = strtok(NULL, " ");icount++;
			icount++;
		}

		if (strcasecmp(suffix, "kb") == 0)
		{
			value = (uint64_t)atoll(result);
			value = value * 1024;
		}
		else if (strcasecmp(suffix, "mb") == 0)
		{
			value = (uint64_t)atoll(result);
			value = value * 1024 * 1024;
		}
		else if (strcasecmp(suffix, "gb") == 0)
		{
			value = (uint64_t)atoll(result);
			value = value * 1024 * 1024 * 1024;
		}
		else
			value = (uint64_t)atoll(result);
	}
	return value;
}

/* Function used to check the string is a number or not */
bool stringIsNumber(char *str)
{
	bool ret_val = true;
	int len;
	size_t index;

	if (str == NULL)
		return !ret_val;

	len = strlen(str);
	for(index = 0; index < len; index++)
	{
		if( !isdigit(str[index]))
			return !ret_val;
	}
	return ret_val;
}

/* Function used to trim the string from left only*/
char* leftTrimStr(char* s)
{
	char* new_str = s;

	while (isspace( *new_str))
	{
		++new_str;
	}

	memmove( s, new_str, strlen( new_str) + 1);

	return s;
}

/* Function used to trim the string from right only */
char* rightTrimStr(char* s)
{
	char* end = s + strlen( s);

	while ((end != s) && isspace( *(end-1)))
	{
		--end;
	}

	*end = '\0';

	return s;
}

/* Function used to trim the string from both left and right */
char*  trimStr( char* s)
{
	return rightTrimStr(leftTrimStr(s));
}

bool read_process_status(int *active_processes, int *running_processes,
		int *sleeping_processes, int *stopped_processes, int *zombie_processes, int *total_threads)
{
	FILE          *fpstat;
	DIR           *dirp;
	struct dirent *ent, dbuf;
	char          file_name[MIN_BUFFER_SIZE];
	char          process_type;
	unsigned int  running_threads;

	dirp = opendir(PROC_FILE_SYSTEM_PATH);
	if (!dirp)
	{
		ereport(DEBUG1, (errmsg("Error opening /proc directory")));
		return false;
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

		*active_processes++;

		sprintf(file_name,"/proc/%s/stat", ent->d_name);

		fpstat = fopen(file_name, "r");
		if (fpstat == NULL)
			continue;

		if (fscanf(fpstat, "%*d %*s %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u"
					"%*d %*d %*d %*d %d %*d %*u %*u %*d", &process_type, &running_threads) == EOF)
			ereport(DEBUG1, (errmsg("Error in parsing file '%s'", file_name)));

		if (process_type == 'R')
			*running_processes++;
		else if(process_type == 'S' || process_type == 'D')
			*sleeping_processes++;
		else if (process_type == 'T')
			*stopped_processes++;
		else if (process_type == 'Z')
			*zombie_processes++;
		else
			ereport(DEBUG1, (errmsg("Invalid process type '%c'", process_type)));

		*total_threads = *total_threads + running_threads;

		fclose(fpstat);
		fpstat = NULL;
	}

	closedir(dirp);
	dirp = NULL;

	return true;
}
