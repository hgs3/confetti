/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include "pyconfetti.h"

// Constructor.
static int Confetti_init(PyConfetti *self, PyObject *args, PyObject *kwds)
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
    self->py_confetti = conf_parse(source, &options, &error);
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
static void Confetti_dealloc(PyConfetti *self)
{
    conf_free(self->py_confetti);
    PyObject_Free(self);
}

static PyObject *Confetti_get_root(PyObject *self, PyObject *args)
{
    if (!PyObject_TypeCheck(self, &ConfettiType))
    {
        return NULL;
    }

    PyDirective *instance = PyObject_New(PyDirective, &DirectiveType);
    if (instance == NULL)
    {
        // Handle error
        return NULL;
    }
    PyDirective *dir = instance;
    dir->py_confetti = self;
    dir->data = conf_get_root(((PyConfetti *)self)->py_confetti);
    Py_INCREF(self);
    return (PyObject *)instance;
}

// Define the methods of the class
static PyMethodDef Confetti_methods[] = {
    {"get_root", Confetti_get_root, METH_NOARGS, "Pseudo directive representing the root of the Confetti source text"},
    {NULL}  // Sentinel value
};

// Define the type (class) itself
PyTypeObject ConfettiType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pyconfetti.Confetti",
    .tp_basicsize = sizeof(PyConfetti),
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
