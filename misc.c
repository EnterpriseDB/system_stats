/*------------------------------------------------------------------------
 * misc.c
 *              Miscellaneous utilities
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>

#include "misc.h"


char *str_ltrim(char *s)
{
    while(isspace(*s)) s++;
    return s;
}


char *str_rtrim(char *s)
{
    char *back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}


char *str_trim(char *s)
{
    return str_rtrim(str_ltrim(s));
}


char *remove_quotes(char *s)
{
    if (s[0] == '"' && s[strlen(s) - 1] == '"')
    {
        s[strlen(s) - 1] = '\0';
        return s + 1;
    }
    return s;
}
