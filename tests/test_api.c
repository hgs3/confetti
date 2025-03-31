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

static int callback(void *user_data, conf_mark elem, int argc, const conf_arg *argv)
{
    return 0;
}

TEST(conf_parse, null_arguments)
{
    ASSERT_EQ(CONF_INVALID_OPERATION, conf_parse(NULL, NULL, NULL, NULL, NULL));
}

TEST(conf_parse, null_argument_for_everything_but_string)
{
    ASSERT_EQ(CONF_INVALID_OPERATION, conf_parse("", NULL, NULL, NULL, NULL));
}

TEST(conf_parse, null_argument_for_options_and_error)
{
    ASSERT_EQ(CONF_OK, conf_parse("", NULL, NULL, callback, NULL));
}

TEST(conf_parse, null_string_argument)
{
    conf_err err = {0};
    ASSERT_EQ(CONF_INVALID_OPERATION, conf_parse(NULL, &err, NULL, callback, NULL));
    ASSERT_EQ(CONF_INVALID_OPERATION, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("missing string argument", err.description);
}

TEST(conf_parse, null_function_argument)
{
    conf_err err = {0};
    ASSERT_EQ(CONF_INVALID_OPERATION, conf_parse("", &err, NULL, NULL, NULL));
    ASSERT_EQ(CONF_INVALID_OPERATION, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("missing function argument", err.description);
}

TEST(conf_parse, null_error_argument_on_error)
{
    ASSERT_EQ(CONF_BAD_SYNTAX, conf_parse("{", NULL, NULL, callback, NULL));
}

TEST(conf_parse, bad_format_string)
{
    STUB(vsnprintf, -1);
    conf_err err = {0};
    ASSERT_EQ(CONF_BAD_SYNTAX, conf_parse("}", &err, NULL, callback, NULL));
    ASSERT_EQ(CONF_BAD_SYNTAX, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("formatting description failed", err.description);
}
