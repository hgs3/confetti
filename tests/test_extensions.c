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

static const char *const invalid_punctuators[] = {"{", "}", "\"", ";", "#"};
static const char *const invalid_punctuators_when_expression_arguments_are_enabled[] = {"{", "}", "\"", ";", "#", "(", ")"};

TEST(conf_parse, invalid_punctuator_argument, .iterations=COUNT_OF(invalid_punctuators))
{
    const char *punctuators[] = {invalid_punctuators[TEST_ITERATION], NULL};
    const conf_extensions exts = {
        .punctuator_arguments = punctuators,
    };
    const conf_options opts = {
        .extensions = &exts,
    };
    conf_error err = {0};
    conf_unit *dir = conf_parse("", &opts, &err);
    ASSERT_NULL(dir);
    ASSERT_EQ(CONF_INVALID_OPERATION, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("illegal punctuator argument character", err.description);
    conf_free(dir);
}

TEST(conf_parse, parentheses_are_valid_punctuator_arguments)
{
    const char *punctuators[] = {"(", ")", NULL};
    const conf_extensions exts = {
        .punctuator_arguments = punctuators,
    };
    const conf_options opts = {
        .extensions = &exts,
    };
    conf_error err = {0};
    conf_unit *dir = conf_parse("", &opts, &err);
    ASSERT_NONNULL(dir);
    ASSERT_EQ(CONF_NO_ERROR, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("no error", err.description);
    conf_free(dir);
}

TEST(conf_parse, parentheses_are_invalid_punctuator_arguments, .iterations=COUNT_OF(invalid_punctuators_when_expression_arguments_are_enabled))
{
    const char *punctuators[] = {invalid_punctuators_when_expression_arguments_are_enabled[TEST_ITERATION], NULL};
    const conf_extensions exts = {
        .punctuator_arguments = punctuators,
        .expression_arguments = true, // The parentheses characters are reserved when the expression arguments extension is enabled.
    };
    const conf_options opts = {
        .extensions = &exts,
    };
    conf_error err = {0};
    conf_unit *dir = conf_parse("", &opts, &err);
    ASSERT_NULL(dir);
    ASSERT_EQ(CONF_INVALID_OPERATION, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("illegal punctuator argument character", err.description);
    conf_free(dir);
}

TEST(conf_parse, empty_punctuator_argument_array)
{
    const char *punctuators[] = {NULL};
    const conf_extensions exts = {
        .punctuator_arguments = punctuators,
    };
    const conf_options opts = {
        .extensions = &exts,
    };
    conf_error err = {0};
    conf_unit *dir = conf_parse("", &opts, &err);
    ASSERT_NONNULL(dir);
    ASSERT_EQ(CONF_NO_ERROR, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("no error", err.description);
    conf_free(dir);
}

TEST(conf_parse, only_empty_strings_in_punctuator_arguments)
{
    const char *punctuators[] = {"", "", NULL};
    const conf_extensions exts = {
        .punctuator_arguments = punctuators,
    };
    const conf_options opts = {
        .extensions = &exts,
    };
    conf_error err = {0};
    conf_unit *dir = conf_parse("", &opts, &err);
    ASSERT_NONNULL(dir);
    ASSERT_EQ(CONF_NO_ERROR, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("no error", err.description);
    conf_free(dir);
}

TEST(conf_parse, empty_string_in_punctuator_argument)
{
    const char *punctuators[] = {"+", "", "-", NULL};
    const conf_extensions exts = {
        .punctuator_arguments = punctuators,
    };
    const conf_options opts = {
        .extensions = &exts,
    };
    conf_error err = {0};
    conf_unit *dir = conf_parse("", &opts, &err);
    ASSERT_NONNULL(dir);
    ASSERT_EQ(CONF_NO_ERROR, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("no error", err.description);
    conf_free(dir);
}

TEST(conf_parse, punctuator_argument_with_malformed_unicode)
{
    const char *punctuators[] = {"\xe2\x28\xa1", NULL}; // Invalid 3 Octet Sequence (in 2nd Octet).
    const conf_extensions exts = {
        .punctuator_arguments = punctuators,
    };
    const conf_options opts = {
        .extensions = &exts,
    };
    conf_error err = {0};
    conf_unit *dir = conf_parse("", &opts, &err);
    ASSERT_NULL(dir);
    ASSERT_EQ(CONF_ILLEGAL_BYTE_SEQUENCE, err.code);
    ASSERT_EQ(err.where, 0);
    ASSERT_STR_EQ("punctuator argument with malformed UTF-8", err.description);
    conf_free(dir);
}
