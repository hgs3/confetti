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

static int callback(void *user_data, conf_element elem, int argc, const conf_argument *argv, const conf_comment *comnt)
{
    return 0;
}

TEST(conf_walk, null_arguments)
{
    ASSERT_EQ(CONF_INVALID_OPERATION, conf_walk(NULL, NULL, NULL, NULL));
}

TEST(conf_walk, null_argument_for_everything_but_string)
{
    ASSERT_EQ(CONF_INVALID_OPERATION, conf_walk("", NULL, NULL, NULL));
}

TEST(conf_walk, null_argument_for_options_and_error)
{
    ASSERT_EQ(CONF_NO_ERROR, conf_walk("", NULL, NULL, callback));
}

TEST(conf_walk, null_argument_for_everything_but_string_and_error)
{
    conf_error err = {0};
    ASSERT_EQ(CONF_NO_ERROR, conf_walk("", NULL, &err, callback));
    ASSERT_EQ(CONF_NO_ERROR, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("no error", err.description);
}

TEST(conf_walk, null_string_argument)
{
    conf_error err = {0};
    ASSERT_EQ(CONF_INVALID_OPERATION, conf_walk(NULL, NULL, &err, callback));
    ASSERT_EQ(CONF_INVALID_OPERATION, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("missing string argument", err.description);
}

TEST(conf_walk, null_function_argument)
{
    conf_error err = {0};
    ASSERT_EQ(CONF_INVALID_OPERATION, conf_walk("", NULL, &err, NULL));
    ASSERT_EQ(CONF_INVALID_OPERATION, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("missing function argument", err.description);
}

TEST(conf_walk, null_error_argument_on_error)
{
    ASSERT_EQ(CONF_BAD_SYNTAX, conf_walk("{", NULL, NULL, callback));
}

TEST(conf_walk, low_max_depth)
{
    conf_options opts = {.max_depth = 1};
    conf_error err = {0};
    ASSERT_EQ(CONF_MAX_DEPTH_EXCEEDED, conf_walk("foo { bar { baz }}", &opts, &err, callback));
    ASSERT_EQ(CONF_MAX_DEPTH_EXCEEDED, err.code);
    ASSERT_EQ(err.where, 5);
    ASSERT_STR_EQ("maximum nesting depth exceeded", err.description);
}

TEST(conf_walk, bad_format_string)
{
    STUB(vsnprintf, -1);
    conf_error err = {0};
    ASSERT_EQ(CONF_BAD_SYNTAX, conf_walk("}", NULL, &err, callback));
    ASSERT_EQ(CONF_BAD_SYNTAX, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("formatting description failed", err.description);
}
