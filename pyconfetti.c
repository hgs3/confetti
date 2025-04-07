/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include "confetti.h"

// The Python header uses a #pramga directive on Windows to automatically
// link with the debug version of Python. The problem is the standard Python
// distribution only includes release libraries so, as a workaround, we'll
// trick Python into thinking we're running in release even if we're building
// in debug mode.
#if defined(WIN32) && defined(_DEBUG)
  #undef _DEBUG
  #include <python.h>
  #define _DEBUG
#else
  #include <Python.h>
#endif

typedef struct
{
    PyObject_HEAD
    conf_directive *directive;
} Directive;

// Define the type (class) itself
static PyTypeObject DirectiveType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pyconfetti.Directive",
    .tp_basicsize = sizeof(Directive),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Directive",
    .tp_new = PyType_GenericNew,
};

typedef struct
{
    PyObject_HEAD
    conf_document *document;
} Confetti;

// Constructor.
static int Confetti_init(Confetti *self, PyObject *args, PyObject *kwds)
{
    char *source;
    if (!PyArg_ParseTuple(args, "s", &source))
    {
        return -1;
    }

    conf_extensions extensions = {
        .punctuator_arguments = NULL,
        .c_style_comments = false,
        .expression_arguments = false,
    };

    conf_options options = {
        .extensions = &extensions,
    };

    conf_error error = {0};
    self->document = conf_parse(source, &options, &error);
    if (error.code != CONF_NO_ERROR)
    {
        if (error.code == CONF_BAD_SYNTAX)
        {
            PyErr_SetString(PyExc_ValueError, error.description);
        }
        else if (error.code == CONF_ILLEGAL_BYTE_SEQUENCE)
        {
            PyErr_SetString(PyExc_UnicodeDecodeError, error.description);
        }
        else if (error.code == CONF_OUT_OF_MEMORY)
        {
            PyErr_SetString(PyExc_MemoryError, error.description);
        }
        else
        {
            PyErr_SetString(PyExc_Exception, error.description);
        }
        return -1;
    }
    return 0;
}

// Destructor.
static void Confetti_dealloc(Confetti *self)
{
    conf_free(self->document);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *Confetti_get_root(Confetti *self, PyObject *args)
{
    PyObject *ctor_args = Py_BuildValue("()");
    PyObject *instance = PyObject_CallObject((PyObject *)&DirectiveType, ctor_args);
    Py_DECREF(ctor_args); // Release arguments reference

    if (instance == NULL)
    {
        // Handle error
        return NULL;
    }

    if (!PyObject_TypeCheck(instance, &DirectiveType))
    {
        Py_DECREF(instance);  // Clean up if it's not the correct type
        return NULL;
    }

    Directive *dir = (Directive *)instance;
    dir->directive = conf_get_root(self->document);
    return instance;
}

// Define the methods of the class
static PyMethodDef Confetti_methods[] = {
    {"get_root", (PyCFunction)Confetti_get_root, METH_NOARGS, "Pseudo directive representing the root of the Confetti source text"},
    {NULL}  // Sentinel value
};

// Define the type (class) itself
static PyTypeObject ConfettiType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pyconfetti.Confetti",
    .tp_basicsize = sizeof(Confetti),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)Confetti_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Confetti",
    .tp_methods = Confetti_methods,
    .tp_init = (initproc)Confetti_init,
    .tp_new = PyType_GenericNew,
};

// Module definition
static struct PyModuleDef mymodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "pyconfetti",
    .m_doc = "Confetti configuration language",
    .m_size = -1,
    .m_methods = NULL,
};

PyMODINIT_FUNC PyInit_pyconfetti(void)
{
    PyObject *m;
    
    if (PyType_Ready(&ConfettiType) < 0)
    {
        return NULL;
    }

    if (PyType_Ready(&DirectiveType) < 0)
    {
        return NULL;
    }
    
    m = PyModule_Create(&mymodule);
    if (m == NULL)
    {
        return NULL;
    }

    Py_INCREF(&DirectiveType);
    Py_INCREF(&ConfettiType);

    if (PyModule_AddObject(m, "Confetti", (PyObject *)&ConfettiType) < 0)
    {
        Py_DECREF(&ConfettiType);
        Py_DECREF(&DirectiveType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
