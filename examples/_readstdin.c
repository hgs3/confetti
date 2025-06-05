/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

// This file does NOT contain a Confetti example, but rather a function to read input
// from stdin on both Windows and *nix systems. See the other C files in this directory
// for code examples.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#endif

char *readstdin(void)
{
    char *dynbuf = NULL;
    size_t dynbuf_length = 0;
    size_t dynbuf_capacity = 0;

#if defined(_WIN32)
    const int stdin_fd = _fileno(stdin);
#endif

    for (;;)
    {
        char buffer[4096];
#if defined(_WIN32)
        const int bytes_read = _read(stdin_fd, buffer, sizeof(buffer));
#else
        const ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
#endif
        if (bytes_read == 0)
        {
            break;
        }
        else if (bytes_read < 0)
        {
            perror("read() failed");
            free(dynbuf);
            exit(1);
        }

        const size_t buffer_length = (size_t)bytes_read;
        if ((dynbuf_length + buffer_length) >= dynbuf_capacity)
        {
            size_t newcap = dynbuf_length + buffer_length;
            char *newbuf = realloc(dynbuf, newcap + 1); // +1 for the null byte
            if (newbuf == NULL)
            {
                perror("realloc() failed");
                free(dynbuf);
                exit(1);
            }
            dynbuf = newbuf;
            dynbuf_capacity = newcap;
        }

        memcpy(&dynbuf[dynbuf_length], buffer, buffer_length);
        dynbuf_length += buffer_length;
    }

    if (dynbuf == NULL)
    {
        dynbuf = malloc(sizeof(char));
        if (dynbuf == NULL)
        {
            perror("malloc() failed");
            exit(1);
        }
    }

    dynbuf[dynbuf_length] = '\0';
    return dynbuf;
}

