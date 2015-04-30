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

#ifndef PY_ICAP_CONNECTION_H
#define PY_ICAP_CONNECTION_H

#include <Python.h>

#include <c_icap/request.h>
#include <c_icap/simple_api.h>

// C-ICAP does not have a macro that defines the library version
// so, we rely on a macro only defined in recent versions...
#ifndef ci_allow206 
#define OLD_CICAP_VERSION
#endif

typedef struct
{
    PyObject_HEAD
    char *host;
    int port;
    int proto;
    ci_connection_t *conn;
    ci_request_t *req;
    int req_status;
    PyObject *content;
} PyICAPConnection;

PyTypeObject PyICAPConnectionType;

#endif // PY_ICAP_CONNECTION_H
