/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include "pyconfetti.h"

 // Destructor.
static void Argument_dealloc(PyObject *self)
{
    PyArgument *arg = (PyArgument * )self;
    Py_XDECREF(arg->py_confetti);
    PyObject_Free(arg);
}

static PyObject *Argument_get_value(PyObject *self, void *closure)
{
    if (!PyObject_TypeCheck(self, &ArgumentType))
    {
        return NULL;
    }
    PyArgument *arg = (PyArgument *)self;
    return PyUnicode_FromString(arg->data->value);
}

// Property definition using PyGetSetDef
static PyGetSetDef Argument_getseters[] = {
    {"value", Argument_get_value, NULL, "Value", NULL},
    {NULL},
};

// Define the type (class) itself
PyTypeObject ArgumentType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pyconfetti.Argument",
    .tp_basicsize = sizeof(PyArgument),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Argument",
    .tp_getset = Argument_getseters,
    .tp_new = PyType_GenericNew,
    .tp_dealloc = Argument_dealloc,
};

//////////////////////////////////////////////////////////////////////////

static PyObject *ArgumentIterator_iter(PyObject *self)
{
    // Return self as the iterator.
    Py_INCREF(self);
    return self;
}

static PyObject *ArgumentIterator_next(PyObject *self)
{
    if (!PyObject_TypeCheck(self, &ArgumentIteratorType))
    {
        return NULL;
    }

    PyArgumentIterator *iter = (PyArgumentIterator *)self;
    PyDirective *dir = iter->py_directive;
    if (iter->index >= conf_get_argument_count(dir->data))
    {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    // Create a new Argument.
    PyArgument *arg = PyObject_New(PyArgument, &ArgumentType);
    if (arg == NULL)
    {
        return NULL;
    }
    arg->py_confetti = dir->py_confetti;
    arg->data = conf_get_argument(dir->data, iter->index);
    iter->index += 1;
    Py_INCREF(arg->py_confetti);
    Py_INCREF(dir);
    return (PyObject *)arg;
}

static void ArgumentIterator_dealloc(PyObject *self)
{
    PyArgumentIterator *iter = (PyArgumentIterator *)self;
    Py_XDECREF(iter->py_directive);
    PyObject_Free(iter);
}

// Define the iterator type (class)
PyTypeObject ArgumentIteratorType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pyconfetti.ArgumentIterator",
    .tp_basicsize = sizeof(PyArgumentIterator),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Argument iterator",
    .tp_new = PyType_GenericNew,
    .tp_iter = ArgumentIterator_iter,
    .tp_iternext = ArgumentIterator_next,
    .tp_dealloc = ArgumentIterator_dealloc,
};

