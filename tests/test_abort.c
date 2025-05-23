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

static int callback(void *user_data, conf_element elem, int argc, const conf_argument *argv, const conf_comment *comnt)
{
    int *callbacks_remaining = user_data;
    if (*callbacks_remaining <= 0)
    {
        return 1;
    }

    (*callbacks_remaining) -= 1;
    return 0;
}

TEST(conf_walk, aborted, .iterations=COUNT_OF(tests_utf8))
{
    const struct TestData *td = &tests_utf8[TEST_ITERATION];
    for (int i = 0; i < 1000; i++)
    {
        // Set the total number of successful allocations allowed.
        int tmp = i;

        // Try parsing the input until no memory error is returned.
        conf_options options = {.user_data = &tmp};
        conf_error error = {0};
        const conf_errno code = conf_walk((const char *)td->input, &options, &error, callback);
        if (code != CONF_USER_ABORTED)
        {
            assert(code != CONF_INVALID_OPERATION);
            return;
        }

        ASSERT_EQ(CONF_USER_ABORTED, error.code);
        ASSERT_GTEQ(error.where, 0);
        ASSERT_STR_EQ(error.description, "user aborted");
    }

    ABORT("exceeded allocation failure limit");
}
