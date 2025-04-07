/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include "pyconfetti.h"

static int Arguments_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "cannot instantiate abstract class");
    return 0;
}

static void Argument_dealloc(PyObject *self)
{
    PyArgument *arg = (PyArgument * )self;
    Py_XDECREF(arg->py_confetti);
    PyObject_Free(arg);
}

static PyObject *Argument_get_value(PyObject *self, void *closure)
{
    PyArgument *arg = (PyArgument *)self;
    return PyUnicode_FromString(arg->data->value);
}

static PyObject *Argument_is_expression(PyObject *self, void *closure)
{
    PyArgument *arg = (PyArgument *)self;
    if (arg->data->is_expression)
    {
        return Py_True;
    }
    return Py_False;
}

// Property definition using PyGetSetDef
static PyGetSetDef Argument_getseters[] = {
    {"value", Argument_get_value, NULL, "Value", NULL},
    {"expression", Argument_is_expression, NULL, "Checks if this argument is an expression argument (requires the expressiona argument extension)", NULL},
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
    .tp_init = Arguments_init,
    .tp_new = PyType_GenericNew,
    .tp_dealloc = Argument_dealloc,
};

//////////////////////////////////////////////////////////////////////////

static int ArgumentsIterator_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "cannot instantiate abstract class");
    return 0;
}

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

static Py_ssize_t Argument_length(PyObject *self)
{
    PyArgumentIterator *iter = (PyArgumentIterator *)self;
    return conf_get_argument_count(iter->py_directive->data);
}

static PySequenceMethods ArgumentIterator_as_sequence = {
    .sq_length = Argument_length, // Set the __len__ method.
};

// Define the iterator type (class)
PyTypeObject ArgumentIteratorType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pyconfetti.ArgumentIterator",
    .tp_basicsize = sizeof(PyArgumentIterator),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Argument iterator",
    .tp_init = ArgumentsIterator_init,
    .tp_new = PyType_GenericNew,
    .tp_iter = ArgumentIterator_iter,
    .tp_iternext = ArgumentIterator_next,
    .tp_dealloc = ArgumentIterator_dealloc,
    .tp_as_sequence = &ArgumentIterator_as_sequence, // Set the sequence methods
};

