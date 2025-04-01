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
    ASSERT_NULL(conf_parse_ex(NULL, NULL, NULL, NULL, NULL));
}

TEST(conf_parse, null_string_everything)
{
    ASSERT_NULL(conf_parse_ex(NULL, NULL, NULL, NULL, NULL));
}

TEST(conf_parse, null_argument_for_everything_but_string_and_error)
{
    conf_err err = {0};
    conf_doc *dir = conf_parse_ex("", NULL, &err, NULL, NULL);
    ASSERT_NONNULL(dir);
    ASSERT_EQ(CONF_NO_ERROR, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("no error", err.description);
    conf_free(dir);
}

TEST(conf_parse, null_string_argument)
{
    conf_err err = {0};
    ASSERT_NULL(conf_parse_ex(NULL, NULL, &err, NULL, NULL));
    ASSERT_EQ(CONF_INVALID_OPERATION, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("missing string argument", err.description);
}

TEST(conf_parse, bad_format_string)
{
    STUB(vsnprintf, -1);
    conf_err err = {0};
    ASSERT_NULL(conf_parse_ex("}", NULL, &err, NULL, NULL));
    ASSERT_EQ(CONF_BAD_SYNTAX, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("formatting description failed", err.description);
}

TEST(conf_parse, bad_max_depth)
{
    conf_options opts = {.max_depth = 1};
    conf_err err = {0};
    ASSERT_NULL(conf_parse_ex("foo { bar { baz }}", &opts, &err, NULL, NULL));
    ASSERT_EQ(CONF_MAX_DEPTH_EXCEEDED, err.code);
    ASSERT_EQ(err.where, 5);
    ASSERT_STR_EQ("maximum nesting depth exceeded", err.description);
}

TEST(conf_getdir, null_confetti)
{
    ASSERT_NULL(conf_getdir(NULL, 0));
}

TEST(conf_getdir, out_of_bounds)
{
    conf_doc *doc = conf_parse_ex("foo", NULL, NULL, NULL, NULL);
    ASSERT_NULL(conf_getdir(doc, -1));
    ASSERT_NULL(conf_getdir(doc, 1));
    conf_free(doc);
}

TEST(conf_getremark, out_of_bounds)
{
    conf_doc *doc = conf_parse_ex("# This is a comment.", NULL, NULL, NULL, NULL);
    ASSERT_NULL(conf_getremark(doc, -1));
    ASSERT_NULL(conf_getremark(doc, 1));
    conf_free(doc);
}

TEST(conf_getsubdir, null_confetti)
{
    ASSERT_NULL(conf_getsubdir(NULL, 0));
}

TEST(conf_getsubdir, out_of_bounds)
{
    conf_doc *doc = conf_parse_ex("foo", NULL, NULL, NULL, NULL);
    conf_dir *dir = conf_getdir(doc, 0);
    ASSERT_NONNULL(dir);
    ASSERT_NULL(conf_getsubdir(dir, -1));
    ASSERT_NULL(conf_getsubdir(dir, 1));
    conf_free(doc);
}

TEST(conf_getndir, null_confetti)
{
    ASSERT_EQ(conf_getndir(NULL), 0);
}

TEST(conf_getnsubdir, null_confetti)
{
    ASSERT_EQ(conf_getnsubdir(NULL), 0);
}

TEST(conf_getnarg, null_confetti)
{
    ASSERT_EQ(conf_getnarg(NULL), 0);
}

TEST(conf_getnremark, null_confetti)
{
    ASSERT_EQ(conf_getnremark(NULL), 0);
}

TEST(conf_getarg, out_of_bounds)
{
    conf_doc *doc = conf_parse_ex("foo", NULL, NULL, NULL, NULL);
    conf_dir *dir = conf_getdir(doc, 0);
    ASSERT_NONNULL(dir);
    ASSERT_NULL(conf_getarg(dir, -1));
    ASSERT_NULL(conf_getarg(dir, 1));
    conf_free(doc);
}

