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

typedef void *(*conf_memfn)(void *ud, void *ptr, size_t nsize);

typedef struct conf_state conf_state;
typedef struct conf_dir conf_dir;
typedef struct conf_doc conf_doc;

typedef enum conf_ext
{
    CONF_C_STYLE_COMMENTS = 0x01,
    CONF_EXPRESSION_ARGUMENTS = 0x02,
} conf_ext;

typedef struct conf_options
{
    void *user_data;
    conf_memfn memory_fn;
    conf_ext extensions;
    int max_depth;
} conf_options;

typedef enum conf_errno
{
    CONF_NO_ERROR,
    CONF_OUT_OF_MEMORY,
    CONF_BAD_SYNTAX,
    CONF_ILLEGAL_BYTE_SEQUENCE,
    CONF_INVALID_OPERATION,
    CONF_MAX_DEPTH_EXCEEDED,
    CONF_USER_ABORTED,
} conf_errno;

typedef struct conf_err
{
    size_t where;
    conf_errno code;
    char description[48];
} conf_err;

typedef struct conf_arg
{
    size_t lexeme_offset;
    size_t lexeme_length;
    const char *value;
} conf_arg;

typedef struct conf_comment
{
    size_t offset;
    size_t length;
} conf_comment;

typedef enum conf_elem
{
    CONF_COMMENT,
    CONF_DIRECTIVE,
    CONF_SUBDIRECTIVE_PUSH,
    CONF_SUBDIRECTIVE_POP,
} conf_elem;

typedef int (*conf_walkfn)(void *ud, conf_elem type, int argc, const conf_arg *argv, const conf_comment *comnt);

conf_errno conf_walk(const char *str, const conf_options *options, conf_err *err, conf_walkfn walk);

conf_doc *conf_parse(const char *string, const conf_options *options, conf_err *error);
void conf_free(conf_doc *doc);

conf_dir *conf_getdir(conf_doc *doc, long index);
long conf_getndir(conf_doc *doc);

conf_comment *conf_getremark(conf_doc *doc, long index);
long conf_getnremark(conf_doc *doc);

conf_dir *conf_getsubdir(conf_dir *directive, long index);
long conf_getnsubdir(conf_dir *directive);

conf_arg *conf_getarg(conf_dir *directive, long index);
long conf_getnarg(conf_dir *directive);

#endif
