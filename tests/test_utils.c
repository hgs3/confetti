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

static void print_tokens(struct StringBuf *sb, conf_directive *dir, int depth)
{
    whitespace(sb, depth);
    strbuf_puts(sb, "command {");

    const long arg_count = conf_get_argument_count(dir);
    if (arg_count > 0)
    {
        for (long i = 0; i < arg_count; i++)
        {
            conf_argument *arg = conf_get_argument(dir, i);

            whitespace(sb, depth+1);
            strbuf_printf(sb, "argument {\n");

            whitespace(sb, depth+2);
            strbuf_printf(sb, "offset %zu\n", arg->lexeme_offset);

            whitespace(sb, depth+2);
            strbuf_printf(sb, "length %zu\n", arg->lexeme_length);

            whitespace(sb, depth+2);
            strbuf_printf(sb, "value \"");
            for (const char *ch = arg->value; *ch; ch++)
            {
                if (*ch == '"')
                {
                    strbuf_printf(sb, "\\"); // Escape the double quotes.
                }
                strbuf_printf(sb, "%c", *ch);
            }
            strbuf_printf(sb, "\"\n");

            if (arg->is_expression)
            {
                whitespace(sb, depth+2);
                strbuf_printf(sb, "expression\n", arg->is_expression);
            }

            whitespace(sb, depth+1);
            strbuf_puts(sb, "}");
        }
    }

    const long subdir_count = conf_get_directive_count(dir);
    if (subdir_count > 0)
    {
        whitespace(sb, depth+1);
        strbuf_puts(sb, "subcommands {");

        for (long i = 0; i < subdir_count; i++)
        {
            print_tokens(sb, conf_get_directive(dir, i), depth+2);
        }

        whitespace(sb, depth+1);
        strbuf_puts(sb, "}");
    }

    whitespace(sb, depth);
    strbuf_puts(sb, "}");
}

static char *tokenize(const char *input, const conf_extensions *extensions)
{
    StringBuf *sb = strbuf_new();

    const conf_options options = {
        .extensions = extensions,
    };

    conf_error error = {0};
    conf_unit *unit = conf_parse(input, &options, &error);
    if (error.code != CONF_NO_ERROR)
    {
        strbuf_printf(sb, "error: %s\n", error.description);
    }
    else
    {
        conf_directive *root = conf_get_root(unit);
        for (long i = 0; i < conf_get_directive_count(root); i++)
        {
            conf_directive *subdir = conf_get_directive(root, i);
            print_tokens(sb, subdir, 0);
        }

        for (long i = 0; i < conf_get_comment_count(unit); i++)
        {
            conf_comment *comment = conf_get_comment(unit, i);
            strbuf_printf(sb, "comment {\n");
            strbuf_printf(sb, "    offset %zu\n", comment->offset);
            strbuf_printf(sb, "    length %zu\n", comment->length);
            strbuf_puts(sb, "}");
        }
    }
    conf_free(unit);
    return strbuf_drop(sb);
}

void compare_snapshots(const char *name, const char *input, const conf_extensions *extensions)
{
    char *actual = tokenize(input, extensions);

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

