/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Henry G. Stratmann III
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License.
 * For full terms see the included LICENSE file.
 */

#ifndef CONFETTI_H
#define CONFETTI_H

#include <stddef.h>
#include <stdbool.h>

typedef void *(*conf_allocfn)(void *user_data, void *ptr, size_t size);

typedef struct conf_unit conf_unit; // Configuration Unit.
typedef struct conf_directive conf_directive; // Configuration Directive.

// This struct is for enabling Confetti extensions as defined in the Annex of the Confetti specification.
typedef struct conf_extensions
{
    const char **punctuator_arguments; // Annex C: Null-terminated list of custom punctuators, e.g. {":=", "+", "-", NULL}
    bool c_style_comments; // Annex A: C family single and mulit-line comment syntax.
    bool expression_arguments; // Annex B: Parenthesized user expression arguments.
} conf_extensions;

typedef struct conf_options
{
    const conf_extensions *extensions;
    conf_allocfn allocator;
    void *user_data;
    int max_depth; // Defaults to 20 (for a "safe" default). Raise or lower as needed.
    bool allow_bidi;
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
    size_t where; // UTF-8 code unit index.
    conf_errno code;
    char description[48];
} conf_error;

typedef struct conf_argument
{
    const char *value;
    size_t lexeme_offset; // UTF-8 code unit index.
    size_t lexeme_length; // UTF-8 code unit count.
    bool is_expression; // True if this is an expression argument extension according to Annex B.
} conf_argument;

typedef struct conf_comment
{
    size_t offset; // UTF-8 code unit index.
    size_t length; // UTF-8 code unit count.
} conf_comment;

typedef enum conf_element
{
    CONF_COMMENT,
    CONF_DIRECTIVE,
    CONF_BLOCK_ENTER,
    CONF_BLOCK_LEAVE,
} conf_element;

typedef int (*conf_walkfn)(void *user_data, conf_element element, int argc, const conf_argument *argv, const conf_comment *comment);

conf_errno conf_walk(const char *string, const conf_options *options, conf_error *error, conf_walkfn walk);

conf_unit *conf_parse(const char *string, const conf_options *options, conf_error *error);
void conf_free(conf_unit *unit);

const conf_comment *conf_get_comment(const conf_unit *unit, long index);
long conf_get_comment_count(const conf_unit *unit);

const conf_directive *conf_get_root(const conf_unit *unit);

const conf_directive *conf_get_directive(const conf_directive *dir, long index);
long conf_get_directive_count(const conf_directive *dir);

const conf_argument *conf_get_argument(const conf_directive *dir, long index);
long conf_get_argument_count(const conf_directive *dir);

#endif
