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
#include <assert.h>
#include <audition.h>

static void *custom_allocf(void *ud, void *ptr, size_t osize, size_t nsize)
{
    if (ptr == NULL)
    {
        assert(osize == 0);
        assert(nsize > 0);

        int *allocs_remaining = ud;
        if (*allocs_remaining <= 0)
        {
            return NULL;
        }

        (*allocs_remaining) -= 1;
        return realloc(ptr, nsize);
    }
    else
    {
        assert(osize > 0);
        assert(nsize == 0);
        free(ptr);
        return NULL;
    }
}

static int callback(void *user_data, conf_mark elem, int argc, const conf_arg *argv)
{
    return 0;
}

TEST(memory, out_of_memory, .iterations=COUNT_OF(tests_utf8))
{
    const struct TestData *td = &tests_utf8[TEST_ITERATION];
    for (int i = 0; i < 1000; i++)
    {
        // Set the total number of successful allocations allowed.
        int tmp = i;

        // Try parsing the input until no memory error is returned.
        conf_err error = {0};
        const conf_errno errno = conf_parse((const char *)td->input, &error, &tmp, callback, custom_allocf);
        if (errno != CONF_NO_MEMORY)
        {
            assert(errno != CONF_INVALID_OPERATION);
            return;
        }

        ASSERT_EQ(CONF_NO_MEMORY, error.code);
        ASSERT_GTEQ(error.where, 0);
        ASSERT_STR_EQ(error.description, "memory allocation failed");
    }

    ABORT("exceeded allocation failure limit");
}

