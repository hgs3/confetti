/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include "confetti.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Allocate a null-terminated C string from the input data.
    char *string = malloc(size + 1);
    memcpy(string, data, size);
    string[size] = '\0';

    conf_err error = {0};
    conf_doc *conf = conf_parse(string, NULL, &error);
    conf_free(conf);
    free(string);
    return 0;
}
