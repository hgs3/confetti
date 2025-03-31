/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include "test_utils.h"
#include "confetti.h"

#include <audition.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

struct StringBuf
{
    int capacity;
    int offset;
    char *buffer;
};

static void resize_if_needed(StringBuf *sb, int needs_chars)
{
    assert(sb);
    assert(needs_chars >= 0);

    if (sb->buffer == NULL)
    {
        // This is the first string being appended, that means the capacity field holds the initial capacity.
        // The initial capacity might not be large enough to contain the incoming string, so compare their lengths.
        int new_capacity = needs_chars + 1;
        if (new_capacity < sb->capacity)
        {
            new_capacity = sb->capacity;
        }
        sb->buffer = malloc(sizeof(sb->buffer[0]) * new_capacity);
        sb->capacity = new_capacity;
    }

    // +1 to account for the NUL character.
    const int chars_remaining = sb->capacity - sb->offset;
    if (needs_chars + 1 >= chars_remaining)
    {
        const int new_capacity = sb->capacity + needs_chars + 1;
        sb->buffer = realloc(sb->buffer, sizeof(sb->buffer[0]) * new_capacity);
        sb->capacity = new_capacity;
    }
}

StringBuf *strbuf_new(void)
{
    StringBuf *sb = calloc(1, sizeof(sb[0]));
    sb->capacity = 1024;
    return sb;
}

void strbuf_free(StringBuf *sb)
{
    if (sb != NULL)
    {
        free(sb->buffer);
        free(sb);
    }
}

void strbuf_clear(StringBuf *sb)
{
    if (sb->buffer != NULL)
    {
        memset(sb->buffer, 0, sb->offset);
    }
    sb->offset = 0;
}

void strbuf_puts(StringBuf *sb, const char *string)
{
    assert(sb);
    assert(string);

    const int string_length = (int)strlen(string);
    resize_if_needed(sb, string_length + 1);

    char *dest = &sb->buffer[sb->offset];
    memcpy(dest, string, sizeof(sb->buffer[0]) * string_length);
    dest[string_length] = '\n';
    dest[string_length + 1] = '\0';
    sb->offset += string_length + 1;
}

void strbuf_printf(StringBuf *sb, const char *format, ...)
{
    // Compute the number of characters needed.
    va_list args, copy;
    va_start(args, format);
    va_copy(copy, args);
    const int chars_needed = vsnprintf(NULL, 0, format, args);
    va_end(args);

    if (chars_needed <= 0)
    {
        return;
    }

    // Append the formatted string.
    resize_if_needed(sb, chars_needed);

    vsnprintf(&sb->buffer[sb->offset], sb->capacity - sb->offset, format, copy);
    va_end(copy);

    sb->offset += chars_needed;
    sb->buffer[sb->offset] = '\0';
}

char *strbuf_drop(StringBuf *sb)
{
    assert(sb);
    char *buffer = sb->buffer;
    free(sb);
    return (buffer != NULL) ? buffer : strdup("");
}

char *readfile(const char *filename)
{
    // Open the file.
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        return NULL;
    }

    // Get the size of the file.
    fseek(fp, 0L, SEEK_END);
    size_t bufsize = ftell(fp);
    rewind(fp);

    // Allocate a buffer large enough to accommodate the file plus null byte.
    char *buf = calloc(1, bufsize + sizeof(uint32_t));
    if (buf != NULL)
    {
        fread(buf, sizeof(buf[0]), bufsize, fp);
    }

    fclose(fp);
    return buf;
}

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
    conf_mark prev;
    struct Directive *parent_directive;
    struct Directive *previous_directive;
};

static int parse_callback(void *user_data, conf_mark elem, int argc, const conf_arg *argv)
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

char *parse(const char *input)
{
    conf_err error = {0};

    struct Directive *dir = calloc(1, sizeof(dir[0]));
    struct UserData ud = {
        .sb = strbuf_new(),
        .depth = 0,
        .prev = -1,
        .parent_directive = dir,
    };

    conf_errno errno = conf_parse(input, &error, &ud, parse_callback, NULL);
    if (errno != CONF_OK)
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

static int print_tokens(void *user_data, conf_mark elem, int argc, const conf_arg *argv)
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

static int print_comments(void *user_data, conf_mark elem, int argc, const conf_arg *argv)
{
    struct UserData *ud = user_data;
    if (elem == CONF_COMMENT)
    {
        assert(argc == 1);
        strbuf_printf(ud->sb, "comment {\n");
        strbuf_printf(ud->sb, "    offset %d\n", argv[0].lexeme_offset);
        strbuf_printf(ud->sb, "    length %d\n", argv[0].lexeme_length);
        strbuf_puts(ud->sb, "}");
    }
    return 0;
}

static char *tokenize(const char *input)
{
    conf_err error = {0};

    struct UserData ud = {
        .sb = strbuf_new(),
        .depth = 0,
        .prev = -1,
    };

    conf_errno errno = conf_parse(input, &error, &ud, print_tokens, NULL);
    if (errno != CONF_OK)
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
        conf_parse(input, &error, &ud, print_comments, NULL);
    }

    return strbuf_drop(ud.sb);
}

void compare_snapshots(const char *name, const char *input)
{
    char *actual = tokenize(input);

    if (!audit_isdir(PATH_TO_SNAPSHOTS))
    {
        if (!audit_mkdir(PATH_TO_SNAPSHOTS))
        {
            ABORT("failed to create snapshots directory: %s", PATH_TO_SNAPSHOTS);
        }
    }

    char path[1024];
    snprintf(path, sizeof(path), PATH_TO_SNAPSHOTS "/%s.txt", name);

    char *expected = readfile(path);
    if (expected == NULL)
    {
        printf("regenerating snapshot: \"%s\"\n", path);
        FILE *fp = fopen(path, "wb");
        if (fp != NULL)
        {
            fputs(actual, fp);
            fclose(fp);
        }
    }
    else
    {
        EXPECT_STR_EQ(expected, actual, "snapshots do not match: \"%s\"\n", name);
    }

    free(actual);
    free(expected);
}

