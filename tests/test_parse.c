/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include "test_utils.h"
#include "test_suite.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <audition.h>

static void whitespace(StringBuf *sb, int depth)
{
    for (int i = 0; i < depth * 4; i++)
    {
        strbuf_printf(sb, " ");
    }
}

static void print_directive(struct StringBuf *sb, conf_directive *dir, int depth)
{
    whitespace(sb, depth);

    const long arg_count = conf_get_argument_count(dir);
    for (long i = 0; i < arg_count; i++)
    {
        conf_argument *arg = conf_get_argument(dir, i);
        strbuf_printf(sb, "<%s>", arg->value);
        if (i < (arg_count - 1))
        {
            strbuf_printf(sb, " ");
        }
    }

    const long subdir_count = conf_get_directive_count(dir);
    if (subdir_count == 0)
    {
        strbuf_puts(sb, "");
        return;
    }

    strbuf_puts(sb, " [");
    for (long i = 0; i < subdir_count; i++)
    {
        print_directive(sb, conf_get_directive(dir, i), depth + 1);
    }

    whitespace(sb, depth);
    strbuf_puts(sb, "]");
}

static char *parse(const char *input, const conf_extensions *extensions)
{
    conf_error error = {0};
    conf_options options = {
        .extensions = extensions,
    };

    StringBuf *sb = strbuf_new();
    conf_document *doc = conf_parse(input, &options, &error);
    if (error.code != CONF_NO_ERROR)
    {
        assert(doc == NULL);
        strbuf_printf(sb, "error: %s\n", error.description);
    }
    else
    {
        assert(doc != NULL);

        conf_directive *root = conf_get_root(doc);
        for (long i = 0; i < conf_get_directive_count(root); i++)
        {
            conf_directive *subdir = conf_get_directive(root, i);
            print_directive(sb, subdir, 0);
        }
        conf_free(doc);
    }
    return strbuf_drop(sb);
}

TEST(parser, pretty_print, .iterations=COUNT_OF(tests_utf8))
{
    const struct TestData *td = &tests_utf8[TEST_ITERATION];
    const char *input = (const char *)td->input;
    const char *output = (const char *)td->output;
    char *actual = parse(input, &td->extensions);
    EXPECT_STR_EQ(output, actual, "snapshots do not match: %s", td->name);
    free(actual);
}

TEST(parser, extract_directives, .iterations=COUNT_OF(tests_utf8))
{
    const struct TestData *td = &tests_utf8[TEST_ITERATION];
    compare_snapshots(td->name, (const char *)td->input, &td->extensions);
}
