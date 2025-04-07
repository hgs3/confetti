/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include "pyconfetti.h"

// Constructor.
static int Directive_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "cannot instantiate abstract class");
    return 0;
}

 // Destructor.
static void Directive_dealloc(PyObject *self)
{
    PyDirective *dir = (PyDirective * )self;
    Py_XDECREF(dir->py_parent_directive);
    Py_XDECREF(dir->py_confetti);
    PyObject_Free(dir);
}

static PyObject *Directive_iter(PyObject *self)
{
    PyDirectiveIterator *iter = PyObject_New(PyDirectiveIterator, &DirectiveIteratorType);
    if (iter == NULL)
    {
        return NULL;
    }
    iter->py_directive = (PyDirective *)self;
    iter->index = 0;
    Py_INCREF(iter->py_directive); // Keep the original object around that the iterator references.
    return (PyObject *)iter;
}

static PyObject *Directive_get_arguments(PyObject *self, void *closure)
{
    PyArgumentIterator *iter = PyObject_New(PyArgumentIterator, &ArgumentIteratorType);
    if (iter == NULL)
    {
        return NULL;
    }
    iter->py_directive = (PyDirective *)self;
    iter->index = 0;
    Py_INCREF(iter->py_directive); // Keep the original object around that the iterator references.
    return (PyObject *)iter;
}

static Py_ssize_t Directive_length(PyObject *self)
{
    PyDirective *dir = (PyDirective *)self;
    return (Py_ssize_t)conf_get_directive_count(dir->data);
}

// Property definition using PyGetSetDef
static PyGetSetDef Directive_getseters[] = {
    {"args", Directive_get_arguments, NULL, "Arguments", NULL},
    {NULL},
};

static PySequenceMethods Directive_as_sequence = {
    .sq_length = Directive_length, // Set the __len__ method.
};

// Define the type (class) itself
PyTypeObject DirectiveType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pyconfetti.Directive",
    .tp_basicsize = sizeof(PyDirective),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Directive",
    .tp_getset = Directive_getseters,
    .tp_init = Directive_init,
    .tp_new = PyType_GenericNew,
    .tp_dealloc = Directive_dealloc,
    .tp_iter = Directive_iter,
    .tp_as_sequence = &Directive_as_sequence,
};

//////////////////////////////////////////////////////////////////////////

static int DirectiveIterator_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "cannot instantiate abstract class");
    return 0;
}

static PyObject *DirectiveIterator_next(PyObject *self)
{
    if (!PyObject_TypeCheck(self, &DirectiveIteratorType))
    {
        return NULL;
    }

    PyDirectiveIterator *iter = (PyDirectiveIterator *)self;
    PyDirective *dir = iter->py_directive;
    if (iter->index >= conf_get_directive_count(dir->data))
    {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    // Create a new directive.
    PyDirective *subdir = PyObject_New(PyDirective, &DirectiveType);
    if (subdir == NULL)
    {
        return NULL;
    }
    subdir->py_parent_directive = (PyObject *)dir;
    subdir->py_confetti = dir->py_confetti;
    subdir->data = conf_get_directive(dir->data, iter->index);
    iter->index += 1;
    Py_INCREF(subdir->py_parent_directive);
    Py_INCREF(subdir->py_confetti);
    Py_INCREF(dir);
    return (PyObject *)subdir;
}

static void DirectiveIterator_dealloc(PyObject *self)
{
    PyDirectiveIterator *iter = (PyDirectiveIterator *)self;
    Py_XDECREF(iter->py_directive);
    PyObject_Free(iter);
}

static Py_ssize_t DirectiveIterator_length(PyObject *self)
{
    PyDirectiveIterator *dir = (PyDirectiveIterator *)self;
    return (Py_ssize_t)conf_get_directive_count(dir->py_directive->data);
}

static PySequenceMethods DirectiveIterator_as_sequence = {
    .sq_length = DirectiveIterator_length, // Set the __len__ method.
};


// Define the iterator type (class)
PyTypeObject DirectiveIteratorType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pyconfetti.DirectiveIterator",
    .tp_basicsize = sizeof(PyDirectiveIterator),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Directive iterator",
    .tp_init = DirectiveIterator_init,
    .tp_new = PyType_GenericNew,
    .tp_iternext = DirectiveIterator_next,
    .tp_dealloc = DirectiveIterator_dealloc,
    .tp_as_sequence = &DirectiveIterator_as_sequence,
};
