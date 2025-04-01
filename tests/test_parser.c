/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include "test_utils.h"
#include "test_suite.h"
#include <stdlib.h>
#include <string.h>
#include <audition.h>


TEST(parser, pretty_print, .iterations=COUNT_OF(tests_utf8))
{
    const struct TestData *td = &tests_utf8[TEST_ITERATION];
    const char *input = (const char *)td->input;
    const char *output = (const char *)td->output;
    char *actual = parse(input);
    EXPECT_STR_EQ(output, actual, "snapshots do not match: %s", td->name);
    free(actual);
}

TEST(parser, extract_directives, .iterations=COUNT_OF(tests_utf8))
{
    const struct TestData *td = &tests_utf8[TEST_ITERATION];
    compare_snapshots(td->name, (const char *)td->input);
}
