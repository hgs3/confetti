/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include "pyconfetti.h"

typedef struct
{
    Py_ssize_t line;
    Py_ssize_t column;
    Py_ssize_t ucs4_offset;
} Location;

Location utf8_to_ucs4(PyObject *ucs4_string, Py_ssize_t utf8_index)
{
    Location location = {
        .line = 1,
        .column = 1,
    };

    // Get the length of the Unicode string (in code points).
    Py_ssize_t utf8_offset = 0;
    const Py_ssize_t ucs4_string_length = PyUnicode_GET_LENGTH(ucs4_string);
    for (Py_ssize_t ucs4_index = 0; ucs4_index < ucs4_string_length; ucs4_index++)
    {
        // Calculate the number of UTF-8 bytes used by the current code point.
        const Py_UCS4 codepoint = PyUnicode_READ_CHAR(ucs4_string, ucs4_index);
        Py_ssize_t utf8_bytes;
        if (codepoint <= 0x7F)
        {
            utf8_bytes = 1;
        }
        else if (codepoint <= 0x7FF)
        {
            utf8_bytes = 2;
        }
        else if (codepoint <= 0xFFFF)
        {
            utf8_bytes = 3;
        }
        else
        {
            utf8_bytes = 4;
        }

        // Update line/column numbers.
        if (codepoint == '\r')
        {
            const Py_UCS4 next_codepoint = PyUnicode_READ_CHAR(ucs4_string, ucs4_index + 1);
            if (next_codepoint == '\n')
            {
                ucs4_index += 1; // Consume the '\r' here, the for-loop will consume the '\n'.
            }
            location.line += 1;
            location.column = 1;
        }
        else
        {
            switch (codepoint)
            {
            case 0x000A: // Line feed
            case 0x000B: // Vertical tab
            case 0x000C: // Form feed
            case 0x0085: // Next line
            case 0x2028: // Line separator
            case 0x2029: // Paragraph separator
                location.line += 1;
                location.column = 1;
                break;
            default:
                location.column += 1;
                break;
            }
        }

        // Check if we've reached or went pass the desired UTF-8 index.
        if (utf8_offset >= utf8_index)
        {
            break;
        }
        location.ucs4_offset = ucs4_index;
        utf8_offset += utf8_bytes;
    }
    return location;
}

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

    // Decode the UTF-8 string, ensuring that it is valid
    PyObject *ucs4_string = PyUnicode_FromString(source);
    if (ucs4_string == NULL)
    {
        PyErr_SetString(PyExc_UnicodeDecodeError, "failed to create a Python string from UTF-8 source code");
        return -1;
    }

    conf_error error = {0};
    self->data = conf_parse(source, &options, &error);
    self->source = strdup(source);
    self->source_ucs4 = ucs4_string;
    if (error.code != CONF_NO_ERROR)
    {
        if (error.code == CONF_BAD_SYNTAX)
        {
            const Location loc = utf8_to_ucs4(ucs4_string, error.where);
            PyErr_SetString(PyExc_SyntaxError, error.description);
            PyErr_SyntaxLocationEx("<confetti>", loc.line, loc.column);
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
    free(self->source);
    conf_free(self->data);
    PyObject_Free(self);
}

static PyObject *Confetti_iter(PyObject *self)
{
    // Allocate an object to represent the root directive.
    PyDirective *root = PyObject_New(PyDirective, &DirectiveType);
    if (root == NULL)
    {
        PyErr_SetString(PyExc_MemoryError, "failed to allocate root directive");
        return NULL;
    }
    root->data = conf_get_root(((PyConfetti *)self)->data);
    root->py_confetti = self;
    Py_INCREF(root->py_confetti);

    // Allocate an iterator to iterate the root directive.
    PyDirectiveIterator *iter = PyObject_New(PyDirectiveIterator, &DirectiveIteratorType);
    if (iter == NULL)
    {
        Py_DECREF(root);
        PyErr_SetString(PyExc_MemoryError, "failed to allocate directive iterator");
        return NULL;
    }
    iter->index = 0;
    iter->py_directive = root;
    Py_INCREF(iter->py_directive); // Keep the original object around that the iterator references.
    return (PyObject *)iter;
}

static PyObject *Confetti_get_comments(PyObject *self, void *closure)
{
    PyCommentIterator *iter = PyObject_New(PyCommentIterator, &CommentIteratorType);
    if (iter == NULL)
    {
        return NULL;
    }
    iter->py_confetti = (PyConfetti *)self;
    iter->index = 0;
    Py_INCREF(iter->py_confetti);
    return (PyObject *)iter;
}

static Py_ssize_t Confetti_length(PyObject *self)
{
    PyConfetti *iter = (PyConfetti *)self;
    return conf_get_directive_count(conf_get_root(iter->data));
}

static PySequenceMethods Confetti_as_sequence = {
    .sq_length = Confetti_length, // Set the __len__ method.
};

static PyGetSetDef Confetti_getseters[] = {
    {"comments", Confetti_get_comments, NULL, "Source comments", NULL},
    {NULL},
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
    .tp_getset = Confetti_getseters,
    .tp_init = (initproc)Confetti_init,
    .tp_iter = Confetti_iter,
    .tp_new = PyType_GenericNew,
    .tp_as_sequence = &Confetti_as_sequence, // Set the sequence methods
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
    
    if ((PyType_Ready(&ConfettiType) < 0) ||
        (PyType_Ready(&CommentType) < 0) ||
        (PyType_Ready(&CommentIteratorType) < 0) ||
        (PyType_Ready(&DirectiveType) < 0) ||
        (PyType_Ready(&DirectiveIteratorType) < 0) ||
        (PyType_Ready(&ArgumentType) < 0) ||
        (PyType_Ready(&ArgumentIteratorType) < 0))
    {
        return NULL;
    }
    
    m = PyModule_Create(&mymodule);
    if (m == NULL)
    {
        return NULL;
    }

    Py_INCREF(&ConfettiType);

    Py_INCREF(&DirectiveType);
    Py_INCREF(&DirectiveIteratorType);

    Py_INCREF(&ArgumentType);
    Py_INCREF(&ArgumentIteratorType);

    if ((PyModule_AddObject(m, "Confetti", (PyObject *)&ConfettiType) < 0) ||
        (PyModule_AddObject(m, "Comment", (PyObject *)&CommentType) < 0) ||
        (PyModule_AddObject(m, "CommentIterator", (PyObject *)&CommentIteratorType) < 0) ||
        (PyModule_AddObject(m, "Directive", (PyObject *)&DirectiveType) < 0) ||
        (PyModule_AddObject(m, "DirectiveIterator", (PyObject *)&DirectiveIteratorType) < 0) ||
        (PyModule_AddObject(m, "Argument", (PyObject *)&ArgumentType) < 0) ||
        (PyModule_AddObject(m, "ArgumentIterator", (PyObject *)&ArgumentIteratorType) < 0))
    {
        Py_DECREF(&ConfettiType);
        Py_DECREF(&CommentType);
        Py_DECREF(&CommentIteratorType);
        Py_DECREF(&DirectiveType);
        Py_DECREF(&DirectiveIteratorType);
        Py_DECREF(&ArgumentType);
        Py_DECREF(&ArgumentIteratorType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
