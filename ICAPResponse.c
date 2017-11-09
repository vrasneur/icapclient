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
#include "ICAPResponse.h"

#include <structmember.h>

#include "gcc_attributes.h"
#include "cicap_compat.h"

// default exception
extern PyObject *PyICAP_Exc;

typedef struct
{
    PyObject *line;
    PyObject *headers;
    size_t idx;
} py_resp_headers_ctx;

static void
py_resp_add_header(void *data, char const *name, char const *value)
{
    py_resp_headers_ctx *ctx = data;

    if(ctx->idx == 0 &&
       (name == NULL || *name == '\0'))
    {
        ctx->line = PyUnicode_FromString(value);
    }
    else
    {
        if(ctx->headers == NULL) {
            ctx->headers = PyList_New(0);
        }

        PyObject *py_header = PyTuple_New(2);
        PyTuple_SET_ITEM(py_header, 0, PyUnicode_FromString(name));
        PyTuple_SET_ITEM(py_header, 1, PyUnicode_FromString(value));
        (void)PyList_Append(ctx->headers, py_header);
    }

    ctx->idx++;
}

static int
py_resp_parse_icap_headers(PyICAPResponse *resp, PyICAPConnection const *conn)
{
    py_resp_headers_ctx ctx = { 0 };
    ci_headers_list_t *icap_headers = conn->req->response_header;

    if(icap_headers == NULL || icap_headers->used <= 0)
    {
        // old versions of C-ICAP delete the headers
        // if we receive a 204 response...
        if(conn->req_status == 204)
        {
            resp->icap_status = PyLong_FromLong(conn->req_status);
            resp->icap_reason = PyUnicode_FromString("Unmodified");
        }
        else
        {
            PyErr_SetString(PyICAP_Exc, "No ICAP response line found");
        }

        goto py_resp_parse_icap_headers_error;
    }

    ci_headers_iterate(icap_headers, &ctx, py_resp_add_header);

    if(ctx.line == NULL)
    {
        PyErr_SetString(PyICAP_Exc, "Cannot parse the ICAP response line");
        goto py_resp_parse_icap_headers_error;
    }

    char const *line = PyBytes_AS_STRING(PyUnicode_AsEncodedString(ctx.line, "ASCII", "strict"));
    if(line == NULL)
    {
        PyErr_SetString(PyICAP_Exc, "Cannot get the ICAP response line");
        goto py_resp_parse_icap_headers_error;
    }

    // parse the ICAP response line
    int nread = 0;
    int v1 = 0;
    int v2 = 0;
    int status = 0;
    int res = sscanf(line, "ICAP/%d.%d %d %n", &v1, &v2, &status, &nread);
    if(res != 3)
    {
        PyErr_Format(PyICAP_Exc, "Cannot parse the ICAP response line '%s'", line);
        goto py_resp_parse_icap_headers_error;
    }

    resp->icap_status = PyLong_FromLong(status);
    resp->icap_reason = PyUnicode_FromString(icap_headers->headers[0] + nread);
    resp->icap_headers = ctx.headers;
    ctx.headers = NULL;

py_resp_parse_icap_headers_error:

    Py_XDECREF(ctx.line);
    Py_XDECREF(ctx.headers);

    if(PyErr_Occurred())
    {
        return -1;
    }

    return 0;
}

static void
py_resp_parse_http_req_headers(PyICAPResponse *resp, ci_request_t *req)
{
    ci_headers_list_t *req_headers = ci_http_request_headers(req);

    if(req_headers != NULL && req_headers->used >= 1)
    {
        py_resp_headers_ctx ctx = { 0 };
        ci_headers_iterate(req_headers, &ctx, py_resp_add_header);
        resp->http_req_line = ctx.line;
        resp->http_req_headers = ctx.headers;
    }
}

static void
py_resp_parse_http_resp_headers(PyICAPResponse *resp, ci_request_t *req)
{
    ci_headers_list_t *resp_headers = ci_http_response_headers(req);

    if(resp_headers != NULL && resp_headers->used >= 1)
    {
        py_resp_headers_ctx ctx = { 0 };
        ci_headers_iterate(resp_headers, &ctx, py_resp_add_header);
        resp->http_resp_line = ctx.line;
        resp->http_resp_headers = ctx.headers;
    }
}

static int
py_resp_parse_headers(PyICAPResponse *resp, PyICAPConnection const *conn)
{
    int ret = py_resp_parse_icap_headers(resp, conn);

    if(ret == 0)
    {
        py_resp_parse_http_req_headers(resp, conn->req);
        py_resp_parse_http_resp_headers(resp, conn->req);
    }

    return ret;
}

static void py_resp_init(PyICAPResponse *resp)
{
    resp->icap_status = NULL;
    resp->icap_reason = NULL;
    resp->icap_headers = NULL;
    resp->http_req_line = NULL;
    resp->http_req_headers = NULL;
    resp->http_resp_line = NULL;
    resp->http_resp_headers = NULL;
    resp->content = NULL;
}

PyObject *py_resp_new(PyICAPConnection *conn)
{
    PyICAPResponse *resp = NULL;

    if(conn->req == NULL)
    {
        return NULL;
    }

    resp = PyObject_New(PyICAPResponse, &PyICAPResponseType);
    // set all the custom attributes to NULL
    py_resp_init(resp);

    int ret = py_resp_parse_headers(resp, conn);
    if(ret != 0)
    {
        Py_XDECREF(resp);

        if(!PyErr_Occurred())
        {
            PyErr_SetString(PyICAP_Exc, "Cannot parse the response headers");
        }

        return NULL;
    }

    Py_XINCREF(conn->content);
    resp->content = conn->content;
    return (PyObject *)resp;
}

static PyObject *
py_resp_get_header(PyObject *headers, char const *name)
{
    if(headers != NULL && PyList_Check(headers))
    {
    Py_ssize_t len = PyList_GET_SIZE(headers);
        for(Py_ssize_t idx = 0; idx < len; idx++)
        {
            PyObject *py_header = PyList_GET_ITEM(headers, idx);
            // not a good (name, value) header?
            if(py_header == NULL || !PyTuple_Check(py_header) ||
               PyTuple_GET_SIZE(py_header) != 2)
            {
                continue;
            }

            PyObject *py_name = PyTuple_GET_ITEM(py_header, 0);
            // not a good header key?
            if(py_name == NULL || !PyUnicode_Check(py_name))
            {
                continue;
            }

            char const *header_name = PyBytes_AS_STRING(PyUnicode_AsEncodedString(py_name, "ASCII", "strict"));
            if(header_name != NULL && strcasecmp(header_name, name) == 0)
            {
                PyObject *py_value = PyTuple_GET_ITEM(py_header, 1);
                Py_XINCREF(py_value);

                return py_value;
            }
        }
    }
    Py_RETURN_NONE;
}

static PyObject *
py_resp_get_icap_header(PyICAPResponse *resp, PyObject *args)
{
    char *name = NULL;

    if(!PyArg_ParseTuple(args, "s:get_icap_header", &name))
    {
        return NULL;
    }

    return py_resp_get_header(resp->icap_headers, name);
}

static PyObject *
py_resp_get_http_req_header(PyICAPResponse *resp, PyObject *args)
{
    char *name = NULL;

    if(!PyArg_ParseTuple(args, "s:get_http_req_header", &name))
    {
        return NULL;
    }

    return py_resp_get_header(resp->http_req_headers, name);
}


static PyObject *
py_resp_get_http_resp_header(PyICAPResponse *resp, PyObject *args)
{
    char *name = NULL;

    if(!PyArg_ParseTuple(args, "s:get_http_resp_header", &name))
    {
        return NULL;
    }

    return py_resp_get_header(resp->http_resp_headers, name);
}

static void
py_resp_dealloc(PyObject *self)
{
    PyICAPResponse *resp = (PyICAPResponse *)self;

    Py_XDECREF(resp->icap_status);
    Py_XDECREF(resp->icap_reason);
    Py_XDECREF(resp->icap_headers);
    Py_XDECREF(resp->http_req_line);
    Py_XDECREF(resp->http_req_headers);
    Py_XDECREF(resp->http_resp_line);
    Py_XDECREF(resp->http_resp_headers);
    Py_XDECREF(resp->content);
    Py_TYPE(resp)->tp_free(self);
}

static PyMemberDef py_resp_members[] =
{
    { "icap_status",  T_OBJECT, offsetof(PyICAPResponse, icap_status),
      READONLY, "ICAP response status" },
    { "icap_reason",  T_OBJECT, offsetof(PyICAPResponse, icap_reason),
      READONLY, "ICAP response reason" },
    { "icap_headers",  T_OBJECT, offsetof(PyICAPResponse, icap_headers),
      READONLY, "ICAP response headers" },
    { "http_req_line",  T_OBJECT, offsetof(PyICAPResponse, http_req_line),
      READONLY, "HTTP request line" },
    { "http_req_headers",  T_OBJECT, offsetof(PyICAPResponse, http_req_headers),
      READONLY, "HTTP request headers" },
    { "http_resp_line",  T_OBJECT, offsetof(PyICAPResponse, http_resp_line),
      READONLY, "HTTP response line" },
    { "http_resp_headers",  T_OBJECT, offsetof(PyICAPResponse, http_resp_headers),
      READONLY, "HTTP response headers" },
    { "content",  T_OBJECT, offsetof(PyICAPResponse, content),
      READONLY, "HTTP response content" },
    { .name = NULL }
};

static struct PyMethodDef py_resp_methods[] =
{
    { "get_icap_header", (PyCFunction)py_resp_get_icap_header,
      METH_VARARGS, "Get the value associated with an ICAP response header" },
    { "get_http_req_header", (PyCFunction)py_resp_get_http_req_header,
      METH_VARARGS, "Get the value associated with an HTTP request header" },
    { "get_http_resp_header", (PyCFunction)py_resp_get_http_resp_header,
      METH_VARARGS, "Get the value associated with an HTTP response header" },
    { .ml_name = NULL }
};

PyTypeObject PyICAPResponseType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "icapclient.ICAPResponse",
    sizeof(PyICAPResponse),
    .tp_dealloc = py_resp_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "ICAP Response",
    .tp_members = py_resp_members,
    .tp_methods = py_resp_methods,
    .tp_alloc = PyType_GenericAlloc,
    .tp_free = PyObject_Del
};
