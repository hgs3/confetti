/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025-2026 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include "test_utils.h"
#include "test_suite.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <audition.h>

static void *fallible_allocator(void *ud, void *ptr, size_t size)
{
    if (ptr == NULL)
    {
        assert(size > 0);

        int *allocs_remaining = ud;
        if (*allocs_remaining <= 0)
        {
            return NULL;
        }

        (*allocs_remaining) -= 1;
        return malloc(size);
    }
    else
    {
        assert(size > 0);
        free(ptr);
        return NULL;
    }
}

static int visit(void *user_data, conf_element elem, int argc, const conf_argument *argv, const conf_comment *comnt)
{
    return 0;
}

TEST(memory, parser_out_of_memory, .iterations=COUNT_OF(tests_utf8))
{
    const struct TestData *td = &tests_utf8[TEST_ITERATION];
    for (int i = 0; i < 1000; i++)
    {
        // Set the total number of successful allocations allowed.
        int counter = i;

        // Try parsing the input until no memory error is returned.
        conf_options options = {
            .user_data = &counter,
            .allocator = fallible_allocator,
            .extensions = &td->extensions,
        };
        conf_error error = {0};
        conf_unit *dir = conf_parse((const char *)td->input, &options, &error);
        if (dir != NULL)
        {
            conf_free(dir); // Avoid leakage.
        }

        // Try parsing again without an error structure.
        counter = i;
        dir = conf_parse((const char *)td->input, &options, NULL);
        if (dir != NULL)
        {
            conf_free(dir); // Avoid leakage.
        }

        if (error.code != CONF_OUT_OF_MEMORY)
        {
            assert(error.code != CONF_INVALID_OPERATION);
            return;
        }

        ASSERT_EQ(CONF_OUT_OF_MEMORY, error.code);
        ASSERT_GTEQ(error.where, 0);
        ASSERT_STR_EQ(error.description, "memory allocation failed");
    }

    ABORT("exceeded allocation failure limit");
}

TEST(memory, walker_out_of_memory, .iterations=COUNT_OF(tests_utf8))
{
    const struct TestData *td = &tests_utf8[TEST_ITERATION];
    for (int i = 0; i < 1000; i++)
    {
        // Set the total number of successful allocations allowed.
        int counter = i;

        // Try parsing the input until no memory error is returned.
        conf_options options = {
            .user_data = &counter,
            .allocator = fallible_allocator,
            .extensions = &td->extensions,
        };
        conf_error error = {0};
        const conf_errno code = conf_walk((const char *)td->input, &options, &error, visit);
        if (code != CONF_OUT_OF_MEMORY)
        {
            assert(code != CONF_INVALID_OPERATION);
            return;
        }

        ASSERT_EQ(CONF_OUT_OF_MEMORY, error.code);
        ASSERT_GTEQ(error.where, 0);
        ASSERT_STR_EQ(error.description, "memory allocation failed");
    }

    ABORT("exceeded allocation failure limit");
}

TEST(memory, null_error_structure)
{
    int count = 0;
    conf_options options = {
        .user_data = &count,
        .allocator = fallible_allocator,
    };
    conf_unit *dir = conf_parse("foo", &options, NULL);
    ASSERT_NULL(dir);
}
