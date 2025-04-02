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

typedef struct conf_directive conf_directive;
typedef struct conf_document conf_document;

typedef struct conf_options
{
    void *user_data;
    conf_memfn memory_allocator;
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

typedef struct conf_error
{
    size_t where;
    conf_errno code;
    char description[48];
} conf_error;

typedef struct conf_argument
{
    size_t lexeme_offset;
    size_t lexeme_length;
    const char *value;
} conf_argument;

typedef struct conf_comment
{
    size_t offset;
    size_t length;
} conf_comment;

typedef enum conf_element
{
    CONF_COMMENT,
    CONF_DIRECTIVE,
    CONF_SUBDIRECTIVE_PUSH,
    CONF_SUBDIRECTIVE_POP,
} conf_element;

typedef int (*conf_walkfn)(void *user_data, conf_element element, int argc, const conf_argument *argv, const conf_comment *comment);

conf_errno conf_walk(const char *string, const conf_options *options, conf_error *error, conf_walkfn walk);

conf_document *conf_parse(const char *string, const conf_options *options, conf_error *error);
void conf_free(conf_document *doc);

conf_comment *conf_get_comment(conf_document *doc, long index);
long conf_get_comment_count(conf_document *doc);

conf_directive *conf_get_root(conf_document *doc);

conf_directive *conf_get_directive(conf_directive *directive, long index);
long conf_get_directive_count(conf_directive *directive);

conf_argument *conf_get_argument(conf_directive *directive, long index);
long conf_get_argument_count(conf_directive *directive);

#endif
