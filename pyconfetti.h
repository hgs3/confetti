/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#pragma once

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

extern PyTypeObject ConfettiType;
extern PyTypeObject DirectiveType;
extern PyTypeObject DirectiveIteratorType;

typedef struct
{
    PyObject_HEAD
    conf_document *py_confetti;
} PyConfetti;

typedef struct
{
    PyObject_HEAD
    PyObject *py_confetti;
    PyObject *py_parent_directive;
    conf_directive *data;
} PyDirective;

typedef struct
{
    PyObject_HEAD
        long index;
    PyDirective *py_directive;
} PyDirectiveIterator;

PyMODINIT_FUNC PyInit_pyconfetti(void);
