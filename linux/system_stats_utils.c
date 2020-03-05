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
#include "system_stats.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>

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
