/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

// This source file tests the Confetti public API in ways the other unit tests don't.
// For example, calling a function with a null argument to verify it handles it correctly.

#include "confetti.h"
#include <audition.h>

TEST(conf_parse, null_arguments)
{
    ASSERT_NULL(conf_parse(NULL, NULL, NULL));
}

TEST(conf_parse, null_argument_for_everything_but_string_and_error)
{
    conf_error err = {0};
    conf_unit *dir = conf_parse("", NULL, &err);
    ASSERT_NONNULL(dir);
    ASSERT_EQ(CONF_NO_ERROR, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("no error", err.description);
    conf_free(dir);
}

TEST(conf_parse, null_string_argument)
{
    conf_error err = {0};
    ASSERT_NULL(conf_parse(NULL, NULL, &err));
    ASSERT_EQ(CONF_INVALID_OPERATION, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("missing string argument", err.description);
}

#ifdef DEBUG
TEST(conf_parse, bad_format_string)
{
    STUB(vsnprintf, -1);
    conf_error err = {0};
    ASSERT_NULL(conf_parse("}", NULL, &err));
    ASSERT_EQ(CONF_BAD_SYNTAX, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("formatting description failed", err.description);
}
#endif

TEST(conf_parse, low_max_depth)
{
    conf_options opts = {.max_depth = 1};
    conf_error err = {0};
    ASSERT_NULL(conf_parse("foo { bar { baz }}", &opts, &err));
    ASSERT_EQ(CONF_MAX_DEPTH_EXCEEDED, err.code);
    ASSERT_EQ(err.where, 5);
    ASSERT_STR_EQ("maximum nesting depth exceeded", err.description);
}

TEST(conf_get_root, null_confetti)
{
    ASSERT_NULL(conf_get_root(NULL));
}

TEST(conf_get_comment, out_of_bounds)
{
    conf_unit *unit = conf_parse("# This is a comment.", NULL, NULL);
    ASSERT_NULL(conf_get_comment(unit, -1));
    ASSERT_NULL(conf_get_comment(unit, 1));
    conf_free(unit);
}

TEST(conf_get_comment, null_document)
{
    ASSERT_EQ(conf_get_comment(NULL, 0), 0);
}

TEST(conf_get_directive, null_document)
{
    ASSERT_NULL(conf_get_directive(NULL, 0));
}

TEST(conf_get_directive, out_of_bounds)
{
    conf_unit *unit = conf_parse("foo", NULL, NULL);
    const conf_directive *dir = conf_get_root(unit);
    ASSERT_NONNULL(dir);
    ASSERT_NULL(conf_get_directive(dir, -1));
    ASSERT_NULL(conf_get_directive(dir, 1));
    conf_free(unit);
}

TEST(conf_get_directive_count, null_directive)
{
    ASSERT_EQ(conf_get_directive_count(NULL), 0);
}

TEST(conf_get_argument_count, null_directive)
{
    ASSERT_EQ(conf_get_argument_count(NULL), 0);
}

TEST(conf_get_comment_count, null_document)
{
    ASSERT_EQ(conf_get_comment_count(NULL), 0);
}

TEST(conf_get_argument, out_of_bounds)
{
    conf_unit *unit = conf_parse("foo", NULL, NULL);
    const conf_directive *dir = conf_get_root(unit);
    ASSERT_NONNULL(dir);
    ASSERT_NULL(conf_get_argument(dir, -1));
    ASSERT_NULL(conf_get_argument(dir, 1));
    conf_free(unit);
}

TEST(conf_get_argument, null_directive)
{
    ASSERT_EQ(conf_get_argument(NULL, 0), 0);
}
