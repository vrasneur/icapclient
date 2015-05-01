/* -*- Mode: C; c-basic-offset: 4 -*- */
/*
 * Copyright 2015 Vincent Rasneur <vrasneur@free.fr>
 * All Rights Reserved.
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 3.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 *   NON INFRINGEMENT.  See the GNU General Public License for
 *   more details.
 */

#include "cicap_compat.h"

// if the header iteration function is not defined
// just copy-paste the code from a recent C-ICAP version
#ifdef OLD_CICAP_VERSION
#define eoh(s) ((*s == '\r' && *(s+1) == '\n' && *(s+2) != '\t' && *(s+2) != ' ') || (*s == '\n' && *(s+1) != '\t' && *(s+1) != ' '))

int ci_headers_iterate(ci_headers_list_t * h, void *data, void (*fn)(void *, const char  *head, const char  *value))
{
    char header[256];
    char value[8124];
    char *s;
    size_t i, j;
    for (i = 0; i < (size_t)h->used; i++) {
        s = h->headers[i];
        for (j = 0;  j < sizeof(header)-1 && *s != ':' &&  *s != '\0' && *s != '\r' && *s!='\n'; s++, j++)
            header[j] = *s;
        header[j] = '\0';
        j = 0;
        if (*s == ':') {
            s++;
            while (*s == ' ') s++;
            for (j = 0;  j < sizeof(value)-1 &&  *s != '\0' && !eoh(s); s++, j++)
                value[j] = *s;
        }
        value[j] = '\0';
        fn(data, header, value);
    }
    return 1;
}
#endif
