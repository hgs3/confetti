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
#include "test_utils.h"
#include <audition.h>

static const char *const bidi_chars[] = {
    // BiDi charcters that are also white space according to the Pattern_White_Space property.
    "\u200E",
    "\u200F",

    // Trojan source characters.
    "\u202A",
    "\u202B",
    "\u202D",
    "\u202E",
    "\u2066",
    "\u2067",
    "\u2068",
    "\u202C",
    "\u2069",

    // BiDi characters in unquoted, quoted, and triple quoted arguments.
    "xyz\u2069",
    "\"\u2069\"",
    "\"\"\"\u2069\"\"\"",

    // Escaped BiDi characters in unquoted, quoted, and triple quoted arguments.
    "\\\u2069",
    "\"\\\u2069\"",
    "\"\"\"\\\u2069\"\"\"",

    // BiDi characters in comments.
    "# \u2069",
};

TEST(bidi, allowed, .iterations=COUNT_OF(bidi_chars))
{
    conf_options opts = {.allow_bidi=true};
    conf_error err = {0};
    conf_unit *unit = conf_parse(bidi_chars[TEST_ITERATION], &opts, &err);
    ASSERT_NONNULL(unit);
    conf_free(unit);
}

TEST(bidi, disallowed, .iterations=COUNT_OF(bidi_chars))
{
    conf_options opts = {.allow_bidi=false};
    conf_error err = {0};
    ASSERT_NULL(conf_parse(bidi_chars[TEST_ITERATION], &opts, &err));
    ASSERT_EQ(CONF_BAD_SYNTAX, err.code);
    ASSERT_GTEQ(err.where, 0);
    ASSERT_STR_EQ(err.description, "illegal bidirectional character");
}

//
// Multi-line Comments Extension
//

TEST(bidi, allowed_in_multi_line_comments)
{
    conf_extensions exts = {.c_style_comments=true};
    conf_options opts = {.allow_bidi=true, .extensions=&exts};
    conf_error err = {0};
    conf_unit *unit = conf_parse("/* \u2069 */", &opts, &err);
    ASSERT_NONNULL(unit);
    conf_free(unit);
}

TEST(bidi, disallowed_in_multi_line_comments)
{
    conf_extensions exts = {.c_style_comments=true};
    conf_options opts = {.allow_bidi=false, .extensions=&exts};
    conf_error err = {0};
    ASSERT_NULL(conf_parse("/* \u2069 */", &opts, &err));
    ASSERT_EQ(CONF_BAD_SYNTAX, err.code);
    ASSERT_EQ(err.where, 3);
    ASSERT_STR_EQ(err.description, "illegal bidirectional character");
}

//
// Expression Arguments Extension
//

TEST(bidi, allowed_in_expression_arguments)
{
    conf_extensions exts = {.expression_arguments=true};
    conf_options opts = {.allow_bidi=true, .extensions=&exts};
    conf_error err = {0};
    conf_unit *unit = conf_parse("( \u2069 )", &opts, &err);
    ASSERT_NONNULL(unit);
    conf_free(unit);
}

TEST(bidi, disallowed_in_expression_arguments)
{
    conf_extensions exts = {.expression_arguments=true};
    conf_options opts = {.allow_bidi=false, .extensions=&exts};
    conf_error err = {0};
    ASSERT_NULL(conf_parse("( \u2069 )", &opts, &err));
    ASSERT_EQ(CONF_BAD_SYNTAX, err.code);
    ASSERT_EQ(err.where, 2);
    ASSERT_STR_EQ(err.description, "illegal bidirectional character");
}
