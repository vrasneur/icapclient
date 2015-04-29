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

#include <Python.h>
#include <cStringIO.h>

#include <debug.h>

#include "gcc_attributes.h"
#include "ICAPConnection.h"
#include "ICAPResponse.h"

// PycStringIO is static, use a non-static variable
struct PycStringIO_CAPI *PycStringIO_ref = NULL;

static char icapclient_doc[] = "Provide bindings to the C-ICAP library (Client only)";

// ICAP exception
PyObject *PyICAP_Exc = NULL;

static PyObject *
icapclient_debug_level(GCC_UNUSED PyObject *obj, PyObject *args)
{
    int debug_level = 0;

    if(!PyArg_ParseTuple(args, "i", &debug_level))
    {
	return NULL;
    }

    CI_DEBUG_LEVEL = debug_level;
  
    Py_RETURN_NONE;
}

static PyObject *
icapclient_debug_stdout(GCC_UNUSED PyObject *obj, PyObject *args)
{
    int debug_stdout = 0;

    if(!PyArg_ParseTuple(args, "i", &debug_stdout))
    {
	return NULL;
    }

    CI_DEBUG_STDOUT = debug_stdout;
  
    Py_RETURN_NONE;
}

static struct PyMethodDef icapclient_methods[] =
{
    { "set_debug_level", icapclient_debug_level,
      METH_VARARGS, "set the debug level" },
    { "set_debug_stdout", icapclient_debug_stdout,
      METH_VARARGS, "set the debug to stdout" },
    { .ml_name = NULL }
};

// C-ICAP default cflags include "-fvisibility=hidden"
GCC_STD_DEFAULT
PyMODINIT_FUNC
initicapclient(void)
{
    PyObject *icapclient_module = NULL;

    if(PyType_Ready(&PyICAPConnectionType) < 0)
    {
	return;
    }

    if(PyType_Ready(&PyICAPResponseType) < 0)
    {
	return;
    }
   
    icapclient_module = Py_InitModule3("icapclient", icapclient_methods, icapclient_doc);
    if(icapclient_module == NULL)
    {
	return;
    }

    // some constants for the ICAPConnection object
    PyModule_AddIntConstant(icapclient_module, "AF_INET", AF_INET);
    PyModule_AddIntConstant(icapclient_module, "AF_INET6", AF_INET6);

    // initialize the ICAP exception
    PyICAP_Exc = PyErr_NewException("icapclient.ICAPException", PyExc_IOError, NULL);
    if(PyICAP_Exc == NULL)
    {
	return;
    }

    Py_INCREF(PyICAP_Exc);
    PyModule_AddObject(icapclient_module, "ICAPException", PyICAP_Exc);

    // add the ICAP classes
    Py_INCREF(&PyICAPConnectionType);
    PyModule_AddObject(icapclient_module, "ICAPConnection", (PyObject *)&PyICAPConnectionType);
    Py_INCREF(&PyICAPResponseType);
    PyModule_AddObject(icapclient_module, "ICAPResponse", (PyObject *)&PyICAPResponseType);
   
    // import the cStringIO module
    PycString_IMPORT;
    PycStringIO_ref = PycStringIO;
}
