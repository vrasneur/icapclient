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

#include "cicap_compat.h"
extern PyObject *Python3IO;

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

extern PyTypeObject PyICAPConnectionType;

#endif // PY_ICAP_CONNECTION_H
