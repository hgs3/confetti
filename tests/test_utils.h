/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#pragma once

#include "confetti.h"

typedef struct StringBuf StringBuf;

StringBuf *strbuf_new(void);
void strbuf_free(StringBuf *sb);
void strbuf_puts(StringBuf *sb, const char *s);
void strbuf_printf(StringBuf *sb, const char *format, ...);
char *strbuf_drop(StringBuf *sb);
void strbuf_clear(StringBuf *sb);

char *parse(const char *input);
void compare_snapshots(const char *name, const char *input);

char *readfile(const char *filename);

#ifndef COUNT_OF
#define COUNT_OF(array) (sizeof(array) / sizeof((array)[0]))
#endif
