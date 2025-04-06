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
#include <stdbool.h>

typedef void *(*conf_allocfn)(void *user_data, void *ptr, size_t size);

typedef struct conf_directive conf_directive;
typedef struct conf_document conf_document;

typedef struct conf_extensions
{
    const char **punctuator_arguments;
    bool c_style_comments;
    bool expression_arguments;
} conf_extensions;

typedef struct conf_options
{
    const conf_extensions *extensions;
    conf_allocfn allocator;
    void *user_data;
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
    const char *value;
    size_t lexeme_offset;
    size_t lexeme_length;
    bool is_expression;
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

conf_directive *conf_get_directive(conf_directive *dir, long index);
long conf_get_directive_count(conf_directive *dir);

conf_argument *conf_get_argument(conf_directive *dir, long index);
long conf_get_argument_count(conf_directive *dir);

#endif
