/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

// The implementation of Confetti in C is a tad more complex than it needs to be
// because the API supports both a callback-based "tree" walking API as well as
// an API for building an in-memory "tree" structure.

#include "confetti.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <setjmp.h>
#include <assert.h>
#include <stdint.h>
#include <stdalign.h>
#include <stddef.h>

// This is a workaround for MSVC's lack of max_align_t.
#if defined(_MSC_VER)
#define max_align_t 16
#endif

#define IS_SPACE_CHARACTER 0x1 // includes all new line characters
#define IS_COMMENT_CHARACTER 0x2 // includes white space and new line characters
#define IS_ARGUMENT_CHARACTER 0x4
#define IS_ESCAPABLE_CHARACTER 0x8 // union of argument and reserved punctuator characters

typedef uint32_t uchar;

uint8_t conf_uniflags(uint32_t cp);

typedef enum token_type
{
    TOK_INVALID,
    TOK_EOF,
    TOK_COMMENT,
    TOK_WHITESPACE,
    TOK_NEWLINE,
    TOK_LITERAL,
    TOK_CONTINUATION,
    TOK_SEMICOLON = ';',
    TOK_LCURLYB = '{',
    TOK_RCURLYB = '}',
} token_type;

typedef enum conf_argflag
{
    CONF_QUOTED = 0x1,
    CONF_TRIPLE_QUOTED = 0x2,
} conf_argflag;

typedef struct token
{
    size_t lexeme;
    size_t lexeme_length;
    token_type type;
    conf_argflag flags;
} token;

struct comment
{
    conf_comment data;
    struct comment *next;
};

struct conf_directive
{
    long buffer_length;
    long subdir_count;
    long arguments_count;

    conf_argument *arguments;
    
    conf_directive **subdir;
    conf_directive *subdir_head;
    conf_directive *subdir_tail;
    conf_directive *next;

    char buffer[];
};

struct conf_document
{
    const char *string;
    const char *needle;
    
    conf_walkfn walk;
    conf_directive *root;

    long comments_count;
    struct comment **comments;
    struct comment *comment_head;
    struct comment *comment_tail;
    size_t comment_processed;

    conf_options options;
    token peek;

    jmp_buf err_buf;
    conf_error err;

    alignas(max_align_t) unsigned char padding[sizeof(conf_directive)];
};

static void parse_body(conf_document *conf, conf_directive *parent, int depth);

_Noreturn static void die(conf_document *conf, conf_errno error, const char *where, const char *message, ...)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(where != NULL);
    // LCOV_EXCL_STOP

    conf->err.code = error;
    conf->err.where = where - conf->string;

    va_list args;
    va_start(args, message);
    const int n = vsnprintf(conf->err.description, sizeof(conf->err.description), message, args);
    if (n < 0)
    {
        strcpy(conf->err.description, "formatting description failed");
    }
    va_end(args);
    assert(n < (int)sizeof(conf->err.description)); // LCOV_EXCL_BR_LINE

    longjmp(conf->err_buf, 1);
}

static void *default_alloc(void *ud, void *ptr, size_t size)
{
    assert(size > 0); // LCOV_EXCL_BR_LINE
    if (ptr == NULL)
    {
        return malloc(size);
    }
    else
    {
        free(ptr);
        return NULL;
    }
}

static void *new(conf_document *conf, size_t size)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(size > 0);
    // LCOV_EXCL_STOP
    return conf->options.allocator(conf->options.user_data, NULL, size);
}

static void delete(conf_document *conf, void *ptr, size_t size)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(ptr != NULL);
    assert(size > 0);
    // LCOV_EXCL_STOP
    conf->options.allocator(conf->options.user_data, ptr, size);
}

static uchar utf8decode(conf_document *conf, const char *utf8, size_t *utf8_length)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(utf8 != NULL);
    // LCOV_EXCL_STOP

    static const uint8_t bytes_needed_for_UTF8_sequence[] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        // Defines bit patterns for masking the leading byte of a UTF-8 sequence.
        0,
        0xFF, // Single byte (i.e. fits in ASCII).
        0x1F, // Two byte sequence: 110xxxxx 10xxxxxx.
        0x0F, // Three byte sequence: 1110xxxx 10xxxxxx 10xxxxxx.
        0x07, // Four byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx.
    };

    static const uint8_t next_UTF8_DFA[] = {
        0, 12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72,  // doc 0
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, // doc 1
        12, 0, 12, 12, 12, 12, 12, 0, 12, 0, 12, 12,    // doc 2
        12, 24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12, // doc 3
        12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, // doc 4
        12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12, // doc 5
        12, 12, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, // doc 6
        12, 36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, // doc 7
        12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, // doc 8
    };

    static const uint8_t byte_to_character_class[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        10, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3,
        11, 6, 6, 6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    };

    // Offset to the requested code unit.
    const uint8_t *bytes = (const uint8_t *)utf8;

    // Zero initialize the byte count.
    if (utf8_length != NULL)
    {
        *utf8_length = 0;
    }

    // Check for the end of the string.
    if (bytes[0] == 0x0)
    {
        return '\0';
    }

    // Lookup expected UTF-8 sequence length based on the first byte.
    const int seqlen = bytes_needed_for_UTF8_sequence[bytes[0]];
    if (seqlen == 0)
    {
        goto bad_encoding;
    }

    // Check if the character ends prematurely due to a null terminator.
    for (int i = 1; i < seqlen; i++)
    {
        if (bytes[i] == '\0')
        {
            goto bad_encoding;
        }
    }

    // Consume the first UTF-8 byte.
    uchar value = (uchar)(bytes[0] & bytes_needed_for_UTF8_sequence[256 + seqlen]);

    // Transition to the first DFA doc.
    uint8_t doc = next_UTF8_DFA[byte_to_character_class[bytes[0]]];

    // Consume the remaining UTF-8 bytes.
    for (int i = 1; i < seqlen; i++)
    {
        // Mask off the next byte.
        // It's of the form 10xxxxxx if valid UTF-8.
        value = value << UINT32_C(6) | (uchar)(bytes[i] & UINT8_C(0x3F));

        // Transition to the next DFA doc.
        doc = next_UTF8_DFA[doc + byte_to_character_class[bytes[i]]];
    }

    // Verify the encoded character was well-formed.
    if (doc == UINT8_C(0))
    {
        if (utf8_length != NULL)
        {
            *utf8_length = seqlen;
        }
        return value;
    }
    
bad_encoding:
    die(conf, CONF_ILLEGAL_BYTE_SEQUENCE, utf8, "malformed UTF-8");
}

static bool is_newline(conf_document *conf, const char *string, size_t *length)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(string != NULL);
    assert(length != NULL);
    // LCOV_EXCL_STOP

    if (strncmp(string, "\r\n", 2) == 0)
    {
        *length = 2;
        return true;
    }
    
    switch (utf8decode(conf, string, length))
    {
    case 0x000A: // Line feed
    case 0x000B: // Vertical tab
    case 0x000C: // Form feed
    case 0x000D: // Carriage return
    case 0x0085: // Next line
    case 0x2028: // Line separator
    case 0x2029: // Paragraph separator
        return true;
    }

    return false;
}

static void scan_triple_quoted_literal(conf_document *conf, const char *string, token *tok)
{
    const char *at = string;
    size_t length;

    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(string != NULL);
    assert(tok != NULL);
    assert((at[0] == '"') && (at[1] == '"') && (at[2] == '"'));
    // LCOV_EXCL_STOP

    at += 3; // Skip the opening quote characters.

    for (;;)
    {
        // Check for the end of a triple quoted argument.
        if ((at[0] == '"') && (at[1] == '"') && (at[2] == '"'))
        {
            at += 3;
            break;
        }

        uchar cp = utf8decode(conf, at, &length);
        if (cp == '\0')
        {
            die(conf, CONF_BAD_SYNTAX, at, "unclosed quoted");
        }

        if (cp == '\\')
        {
            at += 1;
            cp = utf8decode(conf, at, &length);
            if ((conf_uniflags(cp) & IS_ESCAPABLE_CHARACTER) == 0)
            {
                if (cp == 0 || is_newline(conf, at, &length))
                {
                    die(conf, CONF_BAD_SYNTAX, at, "incomplete escape sequence");
                }
                die(conf, CONF_BAD_SYNTAX, at, "illegal escape character");
            }
        }
        else
        {
            if (is_newline(conf, at, &length))
            {
                at += length;
                continue;
            }
            else if ((conf_uniflags(cp) & (IS_ESCAPABLE_CHARACTER | IS_SPACE_CHARACTER)) == 0)
            {
                die(conf, CONF_BAD_SYNTAX, at, "illegal character");
            }
        }

        at += length;
    }

    tok->lexeme = string - conf->string;
    tok->lexeme_length = at - string;
    tok->type = TOK_LITERAL;
    tok->flags = CONF_TRIPLE_QUOTED;
}

static void scan_single_quoted_literal(conf_document *conf, const char *string, token *tok)
{
    const char *at = string;
    size_t length;

    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(string != NULL);
    assert(tok != NULL);
    assert(at[0] == '"');
    // LCOV_EXCL_STOP

    at += 1; // Skip the opening quote character.

    for (;;)
    {
        uchar cp = utf8decode(conf, at, &length);

        if (cp == '\0' || is_newline(conf, at, &length))
        {
            die(conf, CONF_BAD_SYNTAX, at, "unclosed quoted");
        }

        if (cp == '\\')
        {
            at += 1;

            // New lines after a backslash are ignored in single quoted arguments.
            if (is_newline(conf, at, &length))
            {
                at += length;
                continue;
            }

            // Verify the backslash is followed by a legal character.
            cp = utf8decode(conf, at, &length);
            if ((conf_uniflags(cp) & IS_ESCAPABLE_CHARACTER) == 0)
            {
                if (cp == 0)
                {
                    die(conf, CONF_BAD_SYNTAX, at, "incomplete escape sequence");
                }
                die(conf, CONF_BAD_SYNTAX, at, "illegal escape character");
            }
        }
        else
        {
            if ((conf_uniflags(cp) & (IS_ESCAPABLE_CHARACTER | IS_SPACE_CHARACTER)) == 0)
            {
                die(conf, CONF_BAD_SYNTAX, at, "illegal character");
            }

            if (cp == '"')
            {
                at += length;
                break;
            }
        }
        
        at += length;
    }

    tok->lexeme = string - conf->string;
    tok->lexeme_length = at - string;
    tok->type = TOK_LITERAL;
    tok->flags = CONF_QUOTED;
}

static void scan_literal(conf_document *conf, const char *string, token *tok)
{
    const char *at = string;
    size_t length;

    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(string != NULL);
    assert(tok != NULL);
    // LCOV_EXCL_STOP
    
    for (;;)
    {
        uchar cp = utf8decode(conf, at, &length);
        if (cp == '\\')
        {
            at += 1;
            cp = utf8decode(conf, at, &length);
            if ((conf_uniflags(cp) & IS_ESCAPABLE_CHARACTER) == 0)
            {
                die(conf, CONF_BAD_SYNTAX, at, "illegal escape character");
            }
        }
        else if ((conf_uniflags(cp) & IS_ARGUMENT_CHARACTER) == 0)
        {
            break;
        }
        
        at += length;
    }

    tok->lexeme = string - conf->string;
    tok->lexeme_length = at - string;
    tok->type = TOK_LITERAL;
    tok->flags = 0;
}

static void scan_whitespace(conf_document *conf, const char *string, token *tok)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(string != NULL);
    assert(tok != NULL);
    // LCOV_EXCL_STOP

    const char *at = string;
    for (;;)
    {
        size_t length;
        const uchar cp = utf8decode(conf, at, &length);
        if (conf_uniflags(cp) & IS_SPACE_CHARACTER)
        {
            at += length;
            continue;
        }
        break;
    }

    tok->lexeme = string - conf->string;
    tok->lexeme_length = at - string;
    tok->type = TOK_WHITESPACE;
    tok->flags = 0;
}

static void scan_comment(conf_document *conf, const char *string, token *tok)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(string != NULL);
    assert(tok != NULL);
    // LCOV_EXCL_STOP

    const char *at = string;
    for (;;)
    {
        if (*at == '\0')
        {
            break;
        }

        size_t length = 0;
        if (is_newline(conf, at, &length))
        {
            break;
        }

        length = 0;
        const uchar cp = utf8decode(conf, at, &length);
        if ((conf_uniflags(cp) & IS_COMMENT_CHARACTER) == 0)
        {
            die(conf, CONF_BAD_SYNTAX, at, "illegal character");
        }

        at += length;
    }

    tok->lexeme = string - conf->string;
    tok->lexeme_length = at - string;
    tok->type = TOK_COMMENT;
    tok->flags = 0;
}

static void scan_token(conf_document *conf, const char *string, token *tok)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(string != NULL);
    assert(tok != NULL);
    // LCOV_EXCL_STOP

    if (string[0] == '#')
    {
        scan_comment(conf, string, tok);
        return;
    }

    if ((string[0] == '{') || (string[0] == '}'))
    {
        tok->type = (token_type)string[0];
        tok->lexeme = string - conf->string;
        tok->lexeme_length = 1;
        tok->flags = 0;
        return;
    }
    
    if (string[0] == '"')
    {
        if ((string[1] == '"') && (string[2] == '"'))
        {
            scan_triple_quoted_literal(conf, string, tok);
        }
        else
        {
            scan_single_quoted_literal(conf, string, tok);
        }
        return;
    }

    if (string[0] == ';')
    {
        tok->type = TOK_SEMICOLON;
        tok->lexeme = string - conf->string;
        tok->lexeme_length = 1;
        tok->flags = 0;
        return;
    }

    if (string[0] == '\\')
    {
        size_t length;
        if (is_newline(conf, &string[1], &length))
        {
            tok->type = TOK_CONTINUATION;
            tok->lexeme = string - conf->string;
            tok->lexeme_length = length + 1;
            tok->flags = 0;
            return;
        }
    }

    if (is_newline(conf, string, &tok->lexeme_length))
    {
        tok->type = TOK_NEWLINE;
        tok->lexeme = string - conf->string;
        tok->flags = 0;
        return;
    }

    const uchar cp = utf8decode(conf, string, NULL);
    if (conf_uniflags(cp) & IS_SPACE_CHARACTER)
    {
        scan_whitespace(conf, string, tok);
        return;
    }

    if (conf_uniflags(cp) & IS_ARGUMENT_CHARACTER)
    {
        scan_literal(conf, string, tok);
        return;
    }

    // For compatibility with source code editing tools that add end-of-file markers, if the last character
    // of the compilation unit is a Control-Z character (U+001A), this character is deleted.
    if (string[0] == 0x1A && string[1] == '\0')
    {
        tok->type = TOK_EOF;
        tok->lexeme = string - conf->string;
        tok->lexeme_length = 0;
        tok->flags = 0;
        return;
    }

    if (cp == 0x0)
    {
        tok->type = TOK_EOF;
        tok->lexeme = string - conf->string;
        tok->lexeme_length = 0;
        tok->flags = 0;
        return;
    }
    
    die(conf, CONF_BAD_SYNTAX, string, "illegal character U+%04X", cp);
}

static void record_comment(conf_document *doc, const conf_comment *data)
{
    struct comment *comment = new(doc, sizeof(comment[0]));
    if (comment == NULL)
    {
        die(doc, CONF_OUT_OF_MEMORY, doc->needle, "memory allocation failed");
    }
    comment->data.offset = data->offset;
    comment->data.length = data->length;
    comment->next = NULL;

    if (doc->comment_head == NULL)
    {
        assert(doc->comment_tail == NULL); // LCOV_EXCL_BR_LINE
        doc->comment_head = comment;
        doc->comment_tail = comment;
    }
    else
    {
        assert(doc->comment_tail != NULL); // LCOV_EXCL_BR_LINE
        doc->comment_tail->next = comment;
        doc->comment_tail = comment;
    }
    doc->comments_count += 1;
}

static token_type peek(conf_document *doc, token *tok)
{
    // LCOV_EXCL_START
    assert(doc != NULL);
    assert(tok != NULL);
    // LCOV_EXCL_STOP

    if (doc->peek.type == TOK_INVALID)
    {
        for (;;)
        {
            scan_token(doc, doc->needle, &doc->peek);
            if (doc->peek.type == TOK_WHITESPACE)
            {
                doc->needle += doc->peek.lexeme_length;
                continue;
            }
            else if (doc->peek.type == TOK_COMMENT)
            {
                // Prevent the same comment from being reported twice.
                // Comments might be parsed twice when the parser
                // is rewound, but they should only be reported
                // once to the user.
                if (doc->comment_processed <= doc->peek.lexeme)
                {
                    const conf_comment comment = {
                        .offset = doc->peek.lexeme,
                        .length = doc->peek.lexeme_length,
                    };
                    if (doc->walk == NULL)
                    {
                        record_comment(doc, &comment);
                    }
                    else if (doc->walk(doc->options.user_data, CONF_COMMENT, 0, NULL, &comment) != 0)
                    {
                        die(doc, CONF_USER_ABORTED, doc->needle, "user aborted");
                    }
                    doc->comment_processed = comment.offset + comment.length;
                }
                doc->needle += doc->peek.lexeme_length;
                continue;
            }
            break;
        }
    }
    *tok = doc->peek;
    return doc->peek.type;
}

static token_type eat(conf_document *conf, token *tok)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(tok != NULL);
    // LCOV_EXCL_STOP

    peek(conf, tok);
    conf->needle += tok->lexeme_length;
    conf->peek.type = TOK_INVALID;
    return tok->type;
}

static size_t copy_literal(conf_document *conf, char *dest, const token *tok)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(tok != NULL);
    // LCOV_EXCL_STOP

    const char *stop_offset = &conf->string[tok->lexeme + tok->lexeme_length];
    const char *offset = &conf->string[tok->lexeme];
    size_t nbytes = 0;

    if (tok->flags & CONF_QUOTED)
    {
        offset += 1; // skip opening quote
        stop_offset -= 1; // discard closing quote
    }

    if (tok->flags & CONF_TRIPLE_QUOTED)
    {
        offset += 3; // skip opening quotes
        stop_offset -= 3; // discard closing quotes
    }

    while (offset < stop_offset)
    {
        if (*offset == '\\')
        {
            offset += 1; // skip the backslash

            // New lines after a backslash are ignored in single quoted arguments.
            if (tok->flags & CONF_QUOTED)
            {
                size_t length;
                if (is_newline(conf, offset, &length))
                {
                    offset += length;
                    continue;
                }
            }
        }

        size_t length = 0;
        utf8decode(conf, offset, &length);
        if (dest != NULL)
        {
            memcpy(dest, offset, length);
            dest += length;
        }

        offset += length;
        nbytes += length;
    }

    return nbytes;
}

//
// Parsing directives is a two step process:
//
//   (1) arguments are first scanned and counted, additionally the total number
//       of characters to represent the arguments is also counted
//
//   (2) arrays large enough to accommodate the arguments and characters is reserved
//       and arguments are re-scanned and their data is copied to these buffer
//
// The point of step #1 is to reduce the number of overall allocations and to avoid the
// use of dynamic array (scanning a quoted literal is cheaper than allocating memory).
//

static void parse_directive(conf_document *conf, conf_directive *parent, int depth)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(depth >= 0);
    // LCOV_EXCL_STOP

    token tok;

    // (1) figure out how much memory is needed for the arguments

    const token saved_peek = conf->peek; // save parser doc
    const char *saved_needle = conf->needle;

    long argument_count = 0;
    long buffer_length = 0;
    for (;;)
    {
        peek(conf, &tok);
        if (tok.type == TOK_LITERAL)
        {
            argument_count += 1;
            buffer_length += copy_literal(conf, NULL, &tok) + 1; // +1 for null byte
            eat(conf, &tok);
        }
        else if (tok.type == TOK_CONTINUATION)
        {
            eat(conf, &tok);
        }
        else
        {
            break;
        }
    }
    conf->peek = saved_peek; // rewind parser doc
    conf->needle = saved_needle;

    // (2) allocate storage for the arguments and copy the data to it

    const size_t size = sizeof(conf_directive) + (size_t)buffer_length;
    conf_directive *dir = new(conf, size);
    if (dir == NULL)
    {
        die(conf, CONF_OUT_OF_MEMORY, conf->needle, "memory allocation failed");
    }
    memset(dir, 0, size);
    dir->buffer_length = buffer_length;

    const long argc = argument_count;
    conf_argument *argv = new(conf, sizeof(argv[0]) * argc);
    if (argv == NULL)
    {
        delete(conf, dir, size);
        die(conf, CONF_OUT_OF_MEMORY, conf->needle, "memory allocation failed");
    }
    dir->arguments = argv;
    dir->arguments_count = argc;

    char *buffer = dir->buffer;
    argument_count = 0;
    for (;;)
    {
        peek(conf, &tok);
        if (tok.type == TOK_LITERAL)
        {
            conf_argument *arg = &argv[argument_count++];
            arg->lexeme_offset = tok.lexeme;
            arg->lexeme_length = tok.lexeme_length;
            arg->value = buffer;
            buffer += copy_literal(conf, buffer, &tok) + 1; // +1 for null byte
            eat(conf, &tok);
        }
        else if (tok.type == TOK_CONTINUATION)
        {
            eat(conf, &tok);
        }
        else
        {
            break;
        }
    }

    // Link this directive with its parent directive.
    if (parent->subdir_head == NULL)
    {
        assert(parent->subdir_tail == NULL); // LCOV_EXCL_BR_LINE
        parent->subdir_head = dir;
        parent->subdir_tail = dir;
    }
    else
    {
        assert(parent->subdir_tail != NULL); // LCOV_EXCL_BR_LINE
        parent->subdir_tail->next = dir;
        parent->subdir_tail = dir;
    }

    // If there is a terminating semicolon, then the can be no subdirectives.
    if (tok.type == ';')
    {
        eat(conf, &tok); // consume ';'
        return;
    }
    
    // Consume as many new lines as possible.
    while (tok.type == TOK_NEWLINE)
    {
        eat(conf, &tok);
        peek(conf, &tok);
    }

    // Check for a subdirective.
    if (tok.type == '{')
    {
        eat(conf, &tok); // consume '{'
        parse_body(conf, dir, depth + 1);

        peek(conf, &tok);
        if (tok.type == '}')
        {
            eat(conf, &tok); // consume '}'
        }
        else
        {
            die(conf, CONF_BAD_SYNTAX, conf->needle, "expected '}'");
        }
    }
}

static void walk_directive(conf_document *conf, int depth)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(depth >= 0);
    // LCOV_EXCL_STOP

    token tok;

    // (1) figure out how much memory is needed for the arguments

    const token saved_peek = conf->peek; // save parser state
    const char *saved_needle = conf->needle;

    int args_count = 0;
    int buffer_length = 0;
    for (;;)
    {
        peek(conf, &tok);
        if (tok.type == TOK_LITERAL)
        {
            args_count += 1;
            buffer_length += copy_literal(conf, NULL, &tok) + 1; // +1 for null byte
            eat(conf, &tok);
        }
        else if (tok.type == TOK_CONTINUATION)
        {
            eat(conf, &tok);
        }
        else
        {
            break;
        }
    }
    conf->peek = saved_peek; // rewind parser state
    conf->needle = saved_needle;

    // (2) allocate storage for the arguments and copy the data to it

    char *args_buffer = new(conf, buffer_length);
    if (args_buffer == NULL)
    {
        die(conf, CONF_OUT_OF_MEMORY, conf->needle, "memory allocation failed");
    }

    const int argc = args_count;
    struct conf_argument *argv = new(conf, argc * sizeof(argv[0]));
    if (argv == NULL)
    {
        delete(conf, args_buffer, buffer_length);
        die(conf, CONF_OUT_OF_MEMORY, conf->needle, "memory allocation failed");
    }

    memset(args_buffer, 0, buffer_length);

    char *buffer = args_buffer;
    args_count = 0;
    for (;;)
    {
        peek(conf, &tok);
        if (tok.type == TOK_LITERAL)
        {
            conf_argument *arg = &argv[args_count++];
            arg->lexeme_offset = tok.lexeme;
            arg->lexeme_length = tok.lexeme_length;
            arg->value = buffer;
            buffer += copy_literal(conf, buffer, &tok) + 1; // +1 for null byte
            eat(conf, &tok);
        }
        else if (tok.type == TOK_CONTINUATION)
        {
            eat(conf, &tok);
        }
        else
        {
            break;
        }
    }

    int r = conf->walk(conf->options.user_data, CONF_DIRECTIVE, argc, argv, NULL);
    delete(conf, argv, argc * sizeof(argv[0]));
    delete(conf, args_buffer, buffer_length);
    if (r != 0)
    {
        die(conf, CONF_USER_ABORTED, conf->needle, "user aborted");
    }

    // If there is a terminating semicolon, then the can be no subdirectives.
    if (tok.type == ';')
    {
        eat(conf, &tok); // consume ';'
        return;
    }

    // Consume as many new lines as possible.
    while (tok.type == TOK_NEWLINE)
    {
        eat(conf, &tok);
        peek(conf, &tok);
    }

    // Check for a subdirective.
    if (tok.type == '{')
    {
        eat(conf, &tok); // consume '{'

        r = conf->walk(conf->options.user_data, CONF_SUBDIRECTIVE_PUSH, 0, NULL, NULL);
        if (r != 0)
        {
            die(conf, CONF_USER_ABORTED, conf->needle, "user aborted");
        }

        parse_body(conf, NULL, depth + 1);

        peek(conf, &tok);
        if (tok.type == '}')
        {
            eat(conf, &tok); // consume '}'

            r = conf->walk(conf->options.user_data, CONF_SUBDIRECTIVE_POP, 0, NULL, NULL);
            if (r != 0)
            {
                die(conf, CONF_USER_ABORTED, conf->needle, "user aborted");
            }
        }
        else
        {
            die(conf, CONF_BAD_SYNTAX, conf->needle, "expected '}'");
        }
    }
}

// Directive lists are parsed in a single pass and collected into a linked list.
// After parsing is complete and the linked list is fully constructed, then the
// list items are copied to an array for O(1) access.
static void parse_body(conf_document *conf, conf_directive *parent, int depth)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(depth >= 0);
    // LCOV_EXCL_STOP

    // Check if the maxmimum nesting depth has been exceeded.
    if (depth >= conf->options.max_depth)
    {
        die(conf, CONF_MAX_DEPTH_EXCEEDED, conf->needle, "maximum nesting depth exceeded");
    }

    // Parse all subdirectives into a linked list.
    long subdirs_count = 0;
    for (;;)
    {
        token tok;
        if (peek(conf, &tok) == TOK_EOF)
        {
            break;
        }

        if (tok.type == TOK_LITERAL)
        {
            if (parent == NULL)
            {
                walk_directive(conf, depth);
                assert(conf->walk != NULL); // LCOV_EXCL_BR_LINE
            }
            else
            {
                parse_directive(conf, parent, depth);
                subdirs_count += 1;
                assert(conf->walk == NULL); // LCOV_EXCL_BR_LINE
            }
            continue;
        }

        if (tok.type == TOK_NEWLINE)
        {
            eat(conf, &tok);
            continue;
        }

        // Check for a subdirective terminator.
        // This will be handled by the caller.
        if (tok.type == '}')
        {
            break;
        }

        if (tok.type == TOK_CONTINUATION)
        {
            die(conf, CONF_BAD_SYNTAX, conf->needle, "unexpected line continuation");
        }

        assert((tok.type == ';') || (tok.type == '{')); // LCOV_EXCL_BR_LINE
        die(conf, CONF_BAD_SYNTAX, conf->needle, "unexpected '%c'", tok.type);
    }

    if (subdirs_count > 0)
    {
        // Allocate an array large enough to accomidate the subdirectives for O(1) access.
        conf_directive **subdirs = new(conf, sizeof(subdirs[0]) * subdirs_count);
        if (subdirs == NULL)
        {
            die(conf, CONF_OUT_OF_MEMORY, conf->needle, "memory allocation failed");
        }
        
        // Copy subdirective pointers to the array.
        long index = 0;
        for (conf_directive *curr = parent->subdir_head; curr != NULL; curr = curr->next)
        {
            subdirs[index] = curr;
            index += 1;
        }
        parent->subdir = subdirs;
        parent->subdir_count = subdirs_count;
    }
}

conf_directive *conf_get_directive(conf_directive *dir, long index)
{
    if (dir == NULL)
    {
        return NULL;
    }
    if (index < 0 || index >= dir->subdir_count)
    {
        return NULL;
    }
    return dir->subdir[index];
}

long conf_get_directive_count(conf_directive *dir)
{
    if (dir == NULL)
    {
        return 0;
    }
    return dir->subdir_count;
}

conf_directive *conf_get_root(conf_document *doc)
{
    if (doc == NULL)
    {
        return NULL;
    }
    return doc->root;
}

conf_argument *conf_get_argument(conf_directive *dir, long index)
{
    if (dir == NULL)
    {
        return NULL;
    }
    if (index < 0 || index >= dir->arguments_count)
    {
        return NULL;
    }
    return &dir->arguments[index];
}

long conf_get_argument_count(conf_directive *dir)
{
    if (dir == NULL)
    {
        return 0;
    }
    return dir->arguments_count;
}

conf_comment *conf_get_comment(conf_document *doc, long index)
{
    if (doc == NULL)
    {
        return NULL;
    }
    if (index < 0 || index >= doc->comments_count)
    {
        return NULL;
    }
    return &doc->comments[index]->data;
}

long conf_get_comment_count(conf_document *doc)
{
    if (doc == NULL)
    {
        return 0;
    }
    return doc->comments_count;
}

static void free_directive(conf_document *conf, conf_directive *dir)
{
    conf_directive *subdir = dir->subdir_head;
    while (subdir != NULL)
    {
        conf_directive *next = subdir->next;
        free_directive(conf, subdir);
        subdir = next;
    }
    
    if (dir->subdir_count > 0)
    {
        delete(conf, dir->subdir, sizeof(dir->subdir[0]) * dir->subdir_count);
    }
    
    assert(dir->arguments_count > 0); // LCOV_EXCL_BR_LINE
    delete(conf, dir->arguments, sizeof(dir->arguments[0]) * dir->arguments_count);
    delete(conf, dir, sizeof(dir[0]) + dir->buffer_length);
}

void conf_free(conf_document *conf)
{
    if (conf == NULL)
    {
        return;
    }
    assert(conf->root != NULL); // LCOV_EXCL_BR_LINE

    conf_directive *root = conf->root;
    conf_directive *dir = root->subdir_head;
    while (dir != NULL)
    {
        conf_directive *next = dir->next;
        free_directive(conf, dir);
        dir = next;
    }

    if (root->subdir_count > 0)
    {
        delete(conf, root->subdir, sizeof(root->subdir[0]) * root->subdir_count);
    }

    if (conf->comments_count > 0)
    {
        struct comment *comment = conf->comment_head;
        while (comment != NULL)
        {
            struct comment *next = comment->next;
            delete(conf, comment, sizeof(comment[0]));
            comment = next;
        }

        if (conf->comments != NULL)
        {
            delete(conf, conf->comments, sizeof(conf->comments[0]) * conf->comments_count);
        }
    }

    delete(conf, conf, sizeof(conf[0]));
}

static void parse_doc(conf_document *doc)
{
    // Skip past the a BOM (byte order mark) character if present.
    if (memchr(doc->needle, '\0', 3) == NULL)
    {
        if (memcmp(doc->needle, "\xEF\xBB\xBF", 3) == 0)
        {
            doc->needle += 3;
        }
    }

    // Parse the Confetti document.
    parse_body(doc, doc->root, 0);

    // Verify the document ended by checking for extraneous tokens.
    token tok;
    peek(doc, &tok);
    if (tok.type != TOK_EOF)
    {
        assert(tok.type == '}'); // LCOV_EXCL_BR_LINE
        die(doc, CONF_BAD_SYNTAX, doc->needle, "found '}' without matching '{'");
    }
}

conf_document *conf_parse(const char *string, const conf_options *options, conf_error *error)
{
    conf_document *doc, tmp = {
        .string = string,
        .needle = string,
        .err.where = -1,
        .err.code = CONF_NO_ERROR,
    };

    if (options != NULL)
    {
        tmp.options = *options;
    }

    if (tmp.options.max_depth < 1)
    {
        tmp.options.max_depth = INT16_MAX; // Default maximum nesting depth.
    }

    if (tmp.options.allocator == NULL)
    {
        tmp.options.allocator = &default_alloc;
    }

    if (string == NULL)
    {
        if (error != NULL)
        {
            error->code = CONF_INVALID_OPERATION;
            strcpy(error->description, "missing string argument");
        }
        return NULL;
    }

    if (setjmp(tmp.err_buf) != 0)
    {
        assert(doc != NULL); // LCOV_EXCL_BR_LINE
        if (error != NULL)
        {
            memcpy(error, &doc->err, sizeof(error[0]));
        }
        conf_free(doc);
        return NULL;
    }

    // Allocate the top-level directive and then begin parsing.
    doc = new(&tmp, sizeof(tmp));
    if (doc == NULL)
    {
        if (error != NULL)
        {
            error->code = CONF_OUT_OF_MEMORY;
            strcpy(error->description, "memory allocation failed");
        }
        return NULL;
    }
    memcpy(doc, &tmp, sizeof(doc[0]));
    doc->root = (conf_directive *)doc->padding;
    parse_doc(doc);

    // Convert the comments linked list to an array for O(1) access.
    if (doc->comments_count > 0)
    {
        struct comment **comments = new(doc, sizeof(comments[0]) * doc->comments_count);
        if (comments == NULL)
        {
            die(doc, CONF_OUT_OF_MEMORY, doc->needle, "memory allocation failed");
        }

        // Copy subdirective pointers to the array.
        long index = 0;
        for (struct comment *curr = doc->comment_head; curr != NULL; curr = curr->next)
        {
            comments[index] = curr;
            index += 1;
        }
        doc->comments = comments;
    }

    if (error != NULL)
    {
        error->where = doc->needle - doc->string;
        error->code = CONF_NO_ERROR;
        strcpy(error->description, "no error");
    }
    return doc;
}

conf_errno conf_walk(const char *string, const conf_options *options, conf_error *error, conf_walkfn walk)
{
    conf_document doc = {
        .string = string,
        .needle = string,
        .walk = walk,
        .err.where = -1,
        .err.code = CONF_NO_ERROR,
    };

    if (options != NULL)
    {
        doc.options = *options;
    }

    if (doc.options.max_depth < 1)
    {
        doc.options.max_depth = INT16_MAX; // Default maximum nesting depth.
    }

    if (doc.options.allocator == NULL)
    {
        doc.options.allocator = &default_alloc;
    }

    if (string == NULL)
    {
        if (error != NULL)
        {
            error->code = CONF_INVALID_OPERATION;
            strcpy(error->description, "missing string argument");
        }
        return CONF_INVALID_OPERATION;
    }

    if (walk == NULL)
    {
        if (error != NULL)
        {
            error->code = CONF_INVALID_OPERATION;
            strcpy(error->description, "missing function argument");
        }
        return CONF_INVALID_OPERATION;
    }

    if (setjmp(doc.err_buf) != 0)
    {
        if (error != NULL)
        {
            memcpy(error, &doc.err, sizeof(error[0]));
        }
    }
    else
    {
        parse_doc(&doc);
        if (error != NULL)
        {
            error->where = doc.needle - doc.string;
            error->code = CONF_NO_ERROR;
            strcpy(error->description, "no error");
        }
    }
    
    return doc.err.code;
}
