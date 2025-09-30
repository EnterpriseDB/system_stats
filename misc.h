/*------------------------------------------------------------------------
 * misc.h
 *              Misc functions
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */
#ifndef MISC_H
#define MISC_H

char *str_ltrim(char *s);
char *str_rtrim(char *s);
char *str_trim(char *s);

char *remove_quotes(char *s);

#endif // MISC_H
