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

#include "ICAPConnection.h"

#include <cStringIO.h>

#include "gcc_attributes.h"
#include "cicap_compat.h"
#include "ICAPResponse.h"

// cStringIO module
extern struct PycStringIO_CAPI *PycStringIO_ref;

// default values
#define ICAP_DEFAULT_PORT 1344
#define ICAP_DEFAULT_SERVICE "avscan"
// in seconds
#define ICAP_DEFAULT_TIMEOUT 300

// default exception
extern PyObject *PyICAP_Exc;

static PyObject *
py_conn_new(PyTypeObject *type, GCC_UNUSED PyObject *args, GCC_UNUSED PyObject *kwds)
{
    PyObject *self = type->tp_alloc(type, 0);
    PyICAPConnection *conn = (PyICAPConnection *)self;

    // should already be set to 0 by the alloc call
    conn->host = NULL;
    conn->port = 0;
    conn->proto = 0;
    conn->conn = NULL;
    conn->req = NULL;
    conn->req_status = 0;
    conn->content = NULL;

    return self;
}

static int
py_conn_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyICAPConnection *conn = (PyICAPConnection *)self;
    char *host = NULL;
    int port = ICAP_DEFAULT_PORT;
    int proto = AF_INET;
   
    static char *kwlist[] = { "host", "port", "proto", NULL };

    if(!PyArg_ParseTupleAndKeywords(args, kwds, "s|ii", kwlist, &host, &port, &proto))
    {
	return -1;
    }

    if(port < 0 || port > 0xffff)
    {
	PyErr_SetString(PyExc_OverflowError, "Port must be 0-65535");
      
	return -1;
    }       

    if(proto != AF_INET && proto != AF_INET6)
    {
	PyErr_SetString(PyExc_ValueError, "Proto must either be AF_INET or AF_INET6");

	return -1;
    }

    conn->host = strdup(host);
    if(conn->host == NULL)
    {
	PyErr_NoMemory();
      
	return -1;
    }
   
    conn->port = port;
    conn->proto = proto;
   
    return 0;
}

static void
py_conn_free_req(PyICAPConnection *conn)
{
    if(conn->req == NULL)
    {
	return;
    }

    // keep the connection, we may need it later
    conn->req->connection = NULL;

    ci_request_destroy(conn->req), conn->req = NULL;

    conn->req_status = 0;

    // destroy the response content too
    if(conn->content != NULL)
    {
	Py_DECREF(conn->content);
	conn->content = NULL;
    }
}

static void
py_conn_free_conn(PyICAPConnection *conn)
{
    if(conn->conn == NULL)
    {
	return;
    }

    close(conn->conn->fd), conn->conn->fd = -1;
    free(conn->conn), conn->conn = NULL;
}

static PyObject *
py_conn_close(PyICAPConnection *conn)
{
    py_conn_free_req(conn);
    py_conn_free_conn(conn);
   
    Py_RETURN_NONE;
}

static void
py_conn_dealloc(PyObject *self)
{
    PyICAPConnection *conn = (PyICAPConnection *)self;

    free(conn->host), conn->host = NULL;
    py_conn_free_req(conn);
    py_conn_free_conn(conn);
   
    Py_TYPE(conn)->tp_free(self);
}

static PyObject *
py_conn_connect(PyICAPConnection *conn)
{
    if(conn->conn == NULL)
    {
	conn->conn = ci_client_connect_to(conn->host, conn->port, conn->proto);
    }
   
    if(conn->conn == NULL)
    {
	PyErr_Format(PyICAP_Exc, "Cannot connect to server '%s:%d'", conn->host, conn->port);
	return NULL;
    }

    Py_RETURN_NONE;
}

static int
py_conn_read(void *ctx, char *buf, int len)
{
    int *fd = ctx;
    int ret = read(*fd, buf, len);
    return ret;
}

static int
py_conn_write(void *ctx, char *buf, int len)
{
    PyObject *sio = ctx;
    int ret = len;

    if(sio != NULL)
    {
	PyGILState_STATE gstate;

	// reacquire the GIL to execute the Python cStringIO code
	gstate = PyGILState_Ensure();
	ret = PycStringIO_ref->cwrite(sio, buf, len);
	PyGILState_Release(gstate);
    }

    return ret;
}

static ci_headers_list_t *
py_conn_build_respmod_http_headers(void)
{
    ci_headers_list_t *resp_headers = ci_headers_create();
    if(resp_headers == NULL)
    {
	return NULL;
    }

    ci_headers_add(resp_headers, "HTTP/1.1 200 OK");
    ci_headers_add(resp_headers, "Transfer-Encoding: chunked");
   
    return resp_headers;
}

static ci_headers_list_t *
py_conn_build_reqmod_http_headers(char const *url)
{
    char *line = NULL;
    int ret = asprintf(&line, "POST %s HTTP/1.1", url);
    if(ret <= -1 || line == NULL)
    {
	return NULL;
    }

    ci_headers_list_t *req_headers = ci_headers_create();
    if(req_headers != NULL)
    {
	ci_headers_add(req_headers, line);
	ci_headers_add(req_headers, "Host: localhost");
	ci_headers_add(req_headers, "Transfer-Encoding: chunked");
    }
   
    free(line), line = NULL;
   
    return req_headers;
}

static int
py_conn_fill_server_options(ci_request_t *req, int timeout)
{
    int ret = CI_OK;
    
    Py_BEGIN_ALLOW_THREADS
    ret = ci_client_get_server_options(req, timeout);
    Py_END_ALLOW_THREADS
    
    if(ret != CI_ERROR)
    {
	// save the retrieved  values;
	int preview = req->preview;
	int allow204 = req->allow204;
#ifndef OLD_CICAP_VERSION
	int allow206 = req->allow206;
#endif
	int keepalive = req->keepalive;
	// reuse the old OPTIONS request
	ci_client_request_reuse(req);
	// copy the saved values
	req->preview = preview;
	req->allow204 = allow204;
#ifndef OLD_CICAP_VERSION
	req->allow206 = allow206;
#endif
	req->keepalive = keepalive;
    }
    
    return ret;
}

static PyObject *
py_conn_request(PyICAPConnection *conn, PyObject *args, PyObject *kwds)
{
    char *type = NULL;
    char *filename = NULL;
    char *service = ICAP_DEFAULT_SERVICE;
    char *url = "/";
    int timeout = ICAP_DEFAULT_TIMEOUT;
    int read_content = 1;
    int input_fd = -1;
    ci_headers_list_t *req_headers = NULL;
    ci_headers_list_t *resp_headers = NULL;
   
    static char *kwlist[] = { "type", "filename", "url", "service", "timeout", "read_content", NULL };

    if(!PyArg_ParseTupleAndKeywords(args, kwds, "ss|ssii:request", kwlist,
				    &type, &filename, &url, &service, &timeout, &read_content))
    {
	goto py_conn_request_error;
    }

    // validate the arguments

    if(strcmp(type, "REQMOD") != 0 && strcmp(type, "RESPMOD") != 0)
    {
	PyErr_SetString(PyExc_ValueError, "Request type should be either 'REQMOD' or 'RESPMOD'");

	goto py_conn_request_error;
    }
   
    if(timeout < 0)
    {
	PyErr_SetString(PyExc_ValueError, "Request timeout must have a positive value (or zero)");
 
	goto py_conn_request_error;
    }

    input_fd = open(filename, O_RDONLY);
    if(input_fd < 0)
    {
	PyErr_SetFromErrnoWithFilename(PyExc_IOError, filename);
      
	goto py_conn_request_error;
    }

    // connect to the server if not already connected
    if(conn->conn == NULL)
    {
	PyObject *res = py_conn_connect(conn);
	Py_XDECREF(res);

	if(PyErr_Occurred())
	{
	    goto py_conn_request_error;
	}
    }
  
    py_conn_free_req(conn);
   
    conn->req = ci_client_request(conn->conn, conn->host, service);
    if(conn->req == NULL)
    {
	PyErr_SetString(PyICAP_Exc, "Cannot create the ICAP request");
      
	goto py_conn_request_error;
    }
   
    int ret = py_conn_fill_server_options(conn->req, timeout);
    if(ret == CI_ERROR)
    {
	PyErr_SetString(PyICAP_Exc, "Cannot send the ICAP OPTIONS request");
      
	goto py_conn_request_error;	
    }

    conn->req->type = (strcmp(type, "REQMOD") == 0) ? ICAP_REQMOD : ICAP_RESPMOD;
    
    req_headers = py_conn_build_reqmod_http_headers(url);
    if(req_headers == NULL)
    {
	PyErr_SetString(PyICAP_Exc, "Cannot create the ICAP HTTP request headers");
               
	goto py_conn_request_error;
    }

    if(conn->req->type == ICAP_RESPMOD)
    {
	resp_headers = py_conn_build_respmod_http_headers();
	if(resp_headers == NULL)
	{
	    PyErr_SetString(PyICAP_Exc, "Cannot create the ICAP HTTP response headers");
               
	    goto py_conn_request_error;
	}
    }

    if(read_content)
    {
	// inhibit an "unused" warning
	(void)PycStringIO;
	conn->content = PycStringIO_ref->NewOutput(128);
    }

    Py_BEGIN_ALLOW_THREADS
    ret = ci_client_icapfilter(conn->req, timeout,
#ifdef OLD_CICAP_VERSION
			       (conn->req->type == ICAP_REQMOD) ? req_headers : resp_headers,
#else
			       req_headers, resp_headers,
#endif
			       &input_fd, py_conn_read,
			       conn->content, py_conn_write);
    Py_END_ALLOW_THREADS
    if(ret == CI_ERROR)
    {
	PyErr_SetString(PyICAP_Exc, "Cannot send the ICAP request");
      
	goto py_conn_request_error;
    }

    conn->req_status = ret;

py_conn_request_error:

    if(req_headers != NULL)
    {
	ci_headers_destroy(req_headers), req_headers = NULL;
    }

    if(resp_headers != NULL)
    {
	ci_headers_destroy(resp_headers), resp_headers = NULL;
    }

    if(input_fd > 0)
    {
	close(input_fd), input_fd = -1;
    }

    if(PyErr_Occurred())
    {
	py_conn_free_req(conn);   

	return NULL;
    }
   
    Py_RETURN_NONE;
}

static PyObject *
py_conn_getresponse(PyICAPConnection *conn)
{
    if(conn->req == NULL)
    {
	PyErr_SetString(PyICAP_Exc, "No ICAP request was sent before");
      
	return NULL;
    }

    PyObject *resp = py_resp_new(conn);
    if(resp == NULL)
    {
	if(!PyErr_Occurred())
	{
	    PyErr_SetString(PyICAP_Exc, "Cannot create the ICAP response object");
	}
      
	return NULL;
    }
   
    return resp;
}

static struct PyMethodDef py_conn_methods[] =
{
    { "connect", (PyCFunction)py_conn_connect,
      METH_NOARGS, "connect to the ICAP server" },
    { "request", (PyCFunction)py_conn_request,
      METH_VARARGS | METH_KEYWORDS, "send an ICAP request" },
    { "getresponse", (PyCFunction)py_conn_getresponse,
      METH_NOARGS, "get the ICAP server response" },
    { "close", (PyCFunction)py_conn_close,
      METH_NOARGS,"close the ICAP connection and free the ICAP request" },
    { .ml_name = NULL }
};


PyTypeObject PyICAPConnectionType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "icapclient.ICAPConnection", 
    sizeof(PyICAPConnection),
    .tp_dealloc = py_conn_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "ICAP connection",
    .tp_methods = py_conn_methods,
    .tp_new = py_conn_new,
    .tp_init = py_conn_init,
    .tp_alloc = PyType_GenericAlloc,
    .tp_free = PyObject_Del
};
