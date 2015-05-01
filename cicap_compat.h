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

#ifndef PY_ICAP_COMPAT_H
#define PY_ICAP_COMPAT_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#include <header.h>
#include <request.h>
#include <simple_api.h>
#pragma GCC diagnostic pop

// C-ICAP does not have a macro that defines the library version
// so, we rely on a macro only defined in recent versions...
#ifndef ci_allow206 
#define OLD_CICAP_VERSION
#endif

#ifdef OLD_CICAP_VERSION
int ci_headers_iterate(ci_headers_list_t * h, void *data, void (*fn)(void *, const char  *head, const char  *value));
#endif

#endif // PY_ICAP_COMPAT_H
