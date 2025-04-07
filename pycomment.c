/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include "pyconfetti.h"

static int Comment_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "cannot instantiate abstract class");
    return 0;
}

static void Comment_dealloc(PyObject *self)
{
    PyComment *comment = (PyComment * )self;
    Py_XDECREF(comment->py_value);
    PyObject_Free(comment);
}

static PyObject *Comment_get_value(PyObject *self, void *closure)
{
    PyComment *comment = (PyComment *)self;
    Py_INCREF(comment->py_value);
    return comment->py_value;
}

static PyObject *Comment_get_offset(PyObject *self, void *closure)
{
    PyComment *comment = (PyComment *)self;
    return PyLong_FromSsize_t(comment->offset);
}

static PyObject *Comment_get_length(PyObject *self, void *closure)
{
    PyComment *comment = (PyComment *)self;
    return PyLong_FromSsize_t(comment->length);
}

// Property definition using PyGetSetDef
static PyGetSetDef Comment_getseters[] = {
    {"value", Comment_get_value, NULL, "Value", NULL},
    {"offset", Comment_get_offset, NULL, "Offset where the comment appeared", NULL},
    {"length", Comment_get_length, NULL, "Length of the comment", NULL},
    {NULL},
};

// Define the type (class) itself
PyTypeObject CommentType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pyconfetti.Comment",
    .tp_basicsize = sizeof(PyComment),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Comment",
    .tp_getset = Comment_getseters,
    .tp_init = Comment_init,
    .tp_new = PyType_GenericNew,
    .tp_dealloc = Comment_dealloc,
};

//////////////////////////////////////////////////////////////////////////

static int CommentIterator_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "cannot instantiate abstract class");
    return 0;
}

static PyObject *CommentIterator_iter(PyObject *self)
{
    Py_INCREF(self);
    return self;
}

static PyObject *CommentIterator_next(PyObject *self)
{
    PyCommentIterator *iter = (PyCommentIterator *)self;
    PyConfetti *conf = iter->py_confetti;
    if (iter->index >= conf_get_comment_count(conf->data))
    {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
    const conf_comment *data = conf_get_comment(conf->data, iter->index);
    iter->index += 1;

    // Compute the offset where the comment begins (used for getting the comments length).
    PyObject *offset = PyUnicode_DecodeUTF8(conf->source, data->offset, "strict");
    if (offset == NULL)
    {
        return NULL;
    }

    // Copy the comment into a null-terminated string.
    PyObject *string = PyUnicode_DecodeUTF8(&conf->source[data->offset], data->length, "strict");
    if (string == NULL)
    {
        Py_DECREF(offset);
        return NULL;
    }

    // Create a new Comment.
    PyComment *comment = PyObject_New(PyComment, &CommentType);
    comment->py_value = string;
    comment->offset = PyUnicode_GET_LENGTH(offset);
    comment->length = PyUnicode_GET_LENGTH(string);
    Py_INCREF(comment->py_value);
    Py_DECREF(offset); // Was just needed to compute the length.
    return (PyObject *)comment;
}

static void CommentIterator_dealloc(PyObject *self)
{
    PyCommentIterator *iter = (PyCommentIterator *)self;
    Py_XDECREF(iter->py_confetti);
    PyObject_Free(iter);
}

static Py_ssize_t CommentIterator_length(PyObject *self)
{
    PyCommentIterator *iter = (PyCommentIterator *)self;
    return conf_get_comment_count(iter->py_confetti->data);
}

static PySequenceMethods CommentIterator_as_sequence = {
    .sq_length = CommentIterator_length, // Set the __len__ method.
};

// Define the iterator type (class)
PyTypeObject CommentIteratorType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pyconfetti.CommentIterator",
    .tp_basicsize = sizeof(PyCommentIterator),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Comment iterator",
    .tp_init = CommentIterator_init,
    .tp_new = PyType_GenericNew,
    .tp_iter = CommentIterator_iter,
    .tp_iternext = CommentIterator_next,
    .tp_dealloc = CommentIterator_dealloc,
    .tp_as_sequence = &CommentIterator_as_sequence, // Set the sequence methods
};
