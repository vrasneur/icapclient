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

#ifndef PY_ICAP_RESPONSE_H
#define PY_ICAP_RESPONSE_H

#include <Python.h>

#include "ICAPConnection.h"

typedef struct
{
    PyObject_HEAD
    PyObject *icap_status;
    PyObject *icap_reason;
    PyObject *icap_headers;
    PyObject *http_req_line;
    PyObject *http_req_headers;
    PyObject *http_resp_line;
    PyObject *http_resp_headers;
    PyObject *content;
} PyICAPResponse;

PyTypeObject PyICAPResponseType;

PyObject *py_resp_new(PyICAPConnection *conn);

#endif // PY_ICAP_RESPONSE_H
