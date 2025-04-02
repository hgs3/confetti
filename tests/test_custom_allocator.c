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

struct Allocation
{
    void *ptr;
    size_t size;
};

static struct Allocation allocations[1024];
static size_t allocations_length;

static void *custom_allocator(void *ud, void *ptr, size_t size)
{
    ASSERT_EQ(ud, &allocations_length, "verify user data is present for malloc/free");

    if (ptr == NULL)
    {
        void *newptr = malloc(size);

        // Record the allocation.
        struct Allocation *alloc = &allocations[allocations_length];
        alloc->ptr = newptr;
        alloc->size = size;
        allocations_length += 1;

        return newptr;
    }
    else
    {
        // Clear the allocation.
        for (size_t i = 0; i < allocations_length; i++)
        {
            struct Allocation *alloc = &allocations[i];
            if (alloc->ptr == ptr)
            {
                ASSERT_EQ(alloc->size, size, "malloc/free size must match");
                alloc->ptr = NULL;
                alloc->size = 0;
                break;
            }
        }
        free(ptr);
        return NULL;
    }
}

TEST_SETUP(memory)
{
    memset(allocations, 0, sizeof(allocations));
    allocations_length = 0;
}

TEST(memory, custom_allocator, .iterations=COUNT_OF(tests_utf8))
{
    const struct TestData *td = &tests_utf8[TEST_ITERATION];
    conf_options options = {
        .user_data = &allocations_length,
        .memory_allocator = custom_allocator,
    };
    conf_error error = {0};
    conf_document *dir = conf_parse((const char *)td->input, &options, &error);
    if (dir != NULL)
    {
        conf_free(dir); // Avoid leakage.
    }

    // Verify all allocations were released.
    // This is a bit redundant, since there are already out-of-memory tests.
    for (size_t i = 0; i < allocations_length; i++)
    {
        EXPECT_NULL(allocations[i].ptr, "allocation %u was not released", i);
    }
}
