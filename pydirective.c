/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include "pyconfetti.h"

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
    if (!PyObject_TypeCheck(self, &DirectiveType))
    {
        return NULL;
    }

    PyDirectiveIterator *iter = PyObject_New(PyDirectiveIterator, &DirectiveIteratorType);
    if (iter == NULL)
    {
        return NULL;
    }
    iter->py_directive = (PyDirective *)self;
    iter->index = 0;
    Py_INCREF(self); // Keep the original object around that the iterator references.
    return (PyObject *)iter;
}

// Define the type (class) itself
PyTypeObject DirectiveType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pyconfetti.PyDirective",
    .tp_basicsize = sizeof(PyDirective),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Directive",
    .tp_new = PyType_GenericNew,
    .tp_dealloc = Directive_dealloc,
    .tp_iter = Directive_iter,
};

//////////////////////////////////////////////////////////////////////////

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
    subdir->data = conf_get_directive(dir->data, iter->index);
    iter->index += 1;
    Py_INCREF(dir);
    return (PyObject *)subdir;
}

static void DirectiveIterator_dealloc(PyObject *self)
{
    PyDirectiveIterator *iter = (PyDirectiveIterator *)self;
    Py_XDECREF(iter->py_directive);
    PyObject_Free(iter);
}

// Define the iterator type (class)
PyTypeObject DirectiveIteratorType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pyconfetti.DirectiveIterator",
    .tp_basicsize = sizeof(PyDirectiveIterator),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Directive iterator",
    .tp_new = PyType_GenericNew,
    .tp_iternext = DirectiveIterator_next,
    .tp_dealloc = DirectiveIterator_dealloc,
};
