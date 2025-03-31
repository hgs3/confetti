/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#ifndef CONFETTI_H
#define CONFETTI_H

#include <stddef.h>

typedef enum conf_errno
{
    CONF_OK,
    CONF_NO_MEMORY,
    CONF_BAD_SYNTAX,
    CONF_ILLEGAL_BYTE_SEQUENCE,
    CONF_INVALID_OPERATION,
    CONF_USER_ABORTED,
} conf_errno;

typedef struct conf_err
{
    long where;
    conf_errno code;
    char description[48];
} conf_err;

typedef struct conf_arg
{
    long lexeme_offset;
    long lexeme_length;
    const char *value;
} conf_arg;

typedef enum conf_mark
{
    CONF_COMMENT,
    CONF_DIRECTIVE,
    CONF_SUBDIRECTIVE_PUSH,
    CONF_SUBDIRECTIVE_POP,
} conf_mark;

typedef void *(*conf_allocfunc)(void *ud, void *ptr, size_t osize, size_t nsize);
typedef int (*conf_parsefunc)(void *ud, conf_mark mark, int argc, const conf_arg *argv);

conf_errno conf_parse(const char *str, conf_err *err, void *ud, conf_parsefunc pf, conf_allocfunc af);

#endif

