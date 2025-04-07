/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>

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

PyMODINIT_FUNC PyInit_pyconfetti(void);

int main(int argc, char *argv[])
{
    FILE *file = NULL;
    PyObject *obj;
    PyStatus status;
    PyConfig config;

    PyImport_AppendInittab("pyconfetti", PyInit_pyconfetti);
    PyConfig_InitPythonConfig(&config);

    // Decode command line arguments and implicitly preinitialize Python.
    status = PyConfig_SetBytesArgv(&config, argc, argv);
    if (PyStatus_Exception(status))
    {
        goto exception;
    }

    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status))
    {
        goto exception;
    }
    PyConfig_Clear(&config);

    // Enter the main Python program.
    return Py_BytesMain(argc, argv);

exception:
    PyConfig_Clear(&config);
    if (PyStatus_IsExit(status))
    {
        return status.exitcode;
    }
    // Display the error message and exit the process with
    // non-zero exit code.
    Py_ExitStatusException(status);
    return 1;
}
