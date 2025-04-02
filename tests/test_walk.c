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

struct Argument
{
    struct Argument *next;
    char *value;
};

struct Directive
{
    struct Argument *arguments;
    struct Argument *arguments_tail;
    struct Directive *subdirectives;
    struct Directive *subdirectives_tail;
    struct Directive *parent;
    struct Directive *next;
};

struct UserData
{
    StringBuf *sb;
    int depth;
    conf_elem prev;
    struct Directive *parent_directive;
    struct Directive *previous_directive;
};

static int walk_callback(void *user_data, conf_elem elem, int argc, const conf_arg *argv, const conf_comment *comnt)
{
    struct UserData *ud = user_data;
    struct Directive *parent = ud->parent_directive;

    if (elem == CONF_DIRECTIVE)
    {
        struct Directive *dir = calloc(1, sizeof(dir[0]));
        for (int i = 0; i < argc; i++)
        {
            struct Argument *arg = calloc(1, sizeof(arg[0]));
            arg->value = strdup(argv[i].value);

            if (dir->arguments == NULL)
            {
                assert(dir->arguments_tail == NULL);
                dir->arguments = arg;
                dir->arguments_tail = arg;
            }
            else
            {
                dir->arguments_tail->next = arg;
                dir->arguments_tail = arg;
            }
        }

        if (parent->subdirectives == NULL)
        {
            parent->subdirectives = dir;
            parent->subdirectives_tail = dir;
        }
        else
        {
            parent->subdirectives_tail->next = dir;
            parent->subdirectives_tail = dir;
        }
        dir->parent = parent;

        ud->previous_directive = dir;
    }

    if (elem == CONF_SUBDIRECTIVE_PUSH)
    {
        assert(ud->previous_directive != NULL);
        ud->parent_directive = ud->previous_directive;
    }

    if (elem == CONF_SUBDIRECTIVE_POP)
    {
        assert(ud->parent_directive != NULL);
        ud->parent_directive = ud->parent_directive->parent;
    }

    ud->prev = elem;
    return 0;
}

static void free_directive(struct Directive *dir)
{
    struct Argument *arg = dir->arguments;
    while (arg != NULL)
    {
        struct Argument *arg_next = arg->next;
        free(arg->value);
        free(arg);
        arg = arg_next;
    }

    struct Directive *subdir = dir->subdirectives;
    while (subdir != NULL)
    {
        struct Directive *subdir_next = subdir->next;
        free_directive(subdir);
        subdir = subdir_next;
    }
    free(dir);
}

static void print_directive(struct StringBuf *sb, struct Directive *dir, int depth)
{
    whitespace(sb, depth);

    for (struct Argument *arg = dir->arguments; arg != NULL; arg = arg->next)
    {
        strbuf_printf(sb, "<%s>", arg->value);
        if (arg->next != NULL)
        {
            strbuf_printf(sb, " ");
        }
    }

    if (dir->subdirectives == NULL)
    {
        strbuf_puts(sb, "");
        return;
    }

    strbuf_puts(sb, " [");
    for (struct Directive *subdir = dir->subdirectives; subdir != NULL; subdir = subdir->next)
    {
        print_directive(sb, subdir, depth + 1);
    }

    whitespace(sb, depth);
    strbuf_puts(sb, "]");
}

static char *walk(const char *input)
{
    conf_err error = {0};

    struct Directive *dir = calloc(1, sizeof(dir[0]));
    struct UserData ud = {
        .sb = strbuf_new(),
        .depth = 0,
        .prev = -1,
        .parent_directive = dir,
    };
    struct conf_options options = {
        .user_data = &ud,
    };

    conf_errno errno = conf_walk(input, &options, &error, walk_callback);
    if (errno != CONF_NO_ERROR)
    {
        strbuf_printf(ud.sb, "error: %s\n", error.description);
    }
    else
    {
        for (struct Directive *subdir = dir->subdirectives; subdir != NULL; subdir = subdir->next)
        {
            print_directive(ud.sb, subdir, 0);
        }
    }
    free_directive(dir);

    return strbuf_drop(ud.sb);
}

static int print_tokens(void *user_data, conf_elem elem, int argc, const conf_arg *argv, const conf_comment *comnt)
{
    struct UserData *ud = user_data;

    if (elem == CONF_COMMENT)
    {
        return 0;
    }

    if (elem == CONF_DIRECTIVE)
    {
        if (ud->prev == CONF_DIRECTIVE)
        {
            whitespace(ud->sb, ud->depth);
            strbuf_puts(ud->sb, "}");
        }

        whitespace(ud->sb, ud->depth);
        strbuf_printf(ud->sb, "command {\n");

        for (int i = 0; i < argc; i++)
        {
            const conf_arg *arg = &argv[i];

            whitespace(ud->sb, ud->depth + 1);
            strbuf_printf(ud->sb, "argument {\n");

            whitespace(ud->sb, ud->depth + 2);
            strbuf_printf(ud->sb, "offset %d\n", arg->lexeme_offset);

            whitespace(ud->sb, ud->depth + 2);
            strbuf_printf(ud->sb, "length %d\n", arg->lexeme_length);

            whitespace(ud->sb, ud->depth + 2);
            strbuf_printf(ud->sb, "value \"");

            for (const char *ch = arg->value; *ch; ch++)
            {
                if (*ch == '"')
                {
                    strbuf_printf(ud->sb, "\\"); // Escape the double quotes.
                }
                strbuf_printf(ud->sb, "%c", *ch);
            }

            strbuf_printf(ud->sb, "\"\n");

            whitespace(ud->sb, ud->depth + 1);
            strbuf_printf(ud->sb, "}\n");
        }
    }

    if (elem == CONF_SUBDIRECTIVE_PUSH)
    {
        ud->depth += 1;
        whitespace(ud->sb, ud->depth);
        strbuf_puts(ud->sb, "subcommands {");
        ud->depth += 1;
    }

    if (elem == CONF_SUBDIRECTIVE_POP)
    {
        if (ud->prev == CONF_DIRECTIVE)
        {
            whitespace(ud->sb, ud->depth);
            strbuf_puts(ud->sb, "}");
        }

        ud->depth -= 1;
        whitespace(ud->sb, ud->depth);
        strbuf_puts(ud->sb, "}");

        ud->depth -= 1;
        whitespace(ud->sb, ud->depth);
        strbuf_puts(ud->sb, "}");
    }

    ud->prev = elem;
    return 0;
}

static int print_comments(void *user_data, conf_elem elem, int argc, const conf_arg *argv, const conf_comment *comnt)
{
    struct UserData *ud = user_data;
    if (elem == CONF_COMMENT)
    {
        assert(argc == 1);
        strbuf_printf(ud->sb, "comment {\n");
        strbuf_printf(ud->sb, "    offset %d\n", comnt->offset);
        strbuf_printf(ud->sb, "    length %d\n", comnt->length);
        strbuf_puts(ud->sb, "}");
    }
    return 0;
}

static char *tokenize(const char *input)
{
    struct UserData ud = {
        .sb = strbuf_new(),
        .depth = 0,
        .prev = -1,
    };

    conf_options options = {
        .user_data = &ud,
    };
    conf_err error = {0};
    conf_errno errno = conf_walk(input, &options, &error, print_tokens);
    if (errno != CONF_NO_ERROR)
    {
        strbuf_clear(ud.sb);
        strbuf_printf(ud.sb, "error: %s\n", error.description);
    }
    else
    {
        if (ud.prev == CONF_DIRECTIVE)
        {
            whitespace(ud.sb, ud.depth);
            strbuf_puts(ud.sb, "}");
        }
        conf_walk(input, &options, &error, print_comments);
    }

    return strbuf_drop(ud.sb);
}

TEST(walker, pretty_print, .iterations=COUNT_OF(tests_utf8))
{
    const struct TestData *td = &tests_utf8[TEST_ITERATION];
    const char *input = (const char *)td->input;
    const char *output = (const char *)td->output;
    char *actual = walk(input);
    EXPECT_STR_EQ(output, actual, "snapshots do not match: %s", td->name);
    free(actual);
}

TEST(walker, extract_directives, .iterations=COUNT_OF(tests_utf8))
{
    const struct TestData *td = &tests_utf8[TEST_ITERATION];
    compare_snapshots(td->name, (const char *)td->input);
}
