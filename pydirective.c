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

static PyObject *Directive_get_subdirectives(PyObject *self, void *closure)
{
    if (!PyObject_TypeCheck(self, &DirectiveType))
    {
        return NULL;
    }

    PySubdirectiveIterator *iter = PyObject_New(PySubdirectiveIterator, &DirectiveIteratorType);
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
    if (!PyObject_TypeCheck(self, &DirectiveType))
    {
        return NULL;
    }

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

// Property definition using PyGetSetDef
static PyGetSetDef Directive_getseters[] = {
    {"subdirs", Directive_get_subdirectives, NULL, "Subdirectives", NULL},
    {"args", Directive_get_arguments, NULL, "Arguments", NULL},
    {NULL},
};

// Define the type (class) itself
PyTypeObject DirectiveType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pyconfetti.Directive",
    .tp_basicsize = sizeof(PyDirective),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Directive",
    .tp_getset = Directive_getseters,
    .tp_new = PyType_GenericNew,
    .tp_dealloc = Directive_dealloc,
};

//////////////////////////////////////////////////////////////////////////

static PyObject *DirectiveIterator_iter(PyObject *self)
{
    // Return self as the iterator.
    Py_INCREF(self);
    return self;
}

static PyObject *DirectiveIterator_next(PyObject *self)
{
    if (!PyObject_TypeCheck(self, &DirectiveIteratorType))
    {
        return NULL;
    }

    PySubdirectiveIterator *iter = (PySubdirectiveIterator *)self;
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
    PySubdirectiveIterator *iter = (PySubdirectiveIterator *)self;
    Py_XDECREF(iter->py_directive);
    PyObject_Free(iter);
}

// Define the iterator type (class)
PyTypeObject DirectiveIteratorType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pyconfetti.DirectiveIterator",
    .tp_basicsize = sizeof(PySubdirectiveIterator),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Directive iterator",
    .tp_new = PyType_GenericNew,
    .tp_iter = DirectiveIterator_iter,
    .tp_iternext = DirectiveIterator_next,
    .tp_dealloc = DirectiveIterator_dealloc,
};
