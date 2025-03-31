/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

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
    long lexeme;
    long lexeme_length;
    token_type type;
    conf_argflag flags;
} token;

typedef struct conf_state
{
    void *user_data;
    conf_allocfunc allocfunc;
    conf_parsefunc parsefunc;

    const char *string;
    const char *needle;
    long comment_processed;

    token peek;

    jmp_buf err_buf;
    conf_err err;
} conf_state;

static void parse_body(conf_state *conf, int depth);

_Noreturn static void die(conf_state *conf, conf_errno error, const char *where, const char *message, ...)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(where != NULL);
    // LCOV_EXCL_STOP

    conf->err.code = error;
    conf->err.where = where - conf->string;

    va_list args;
    va_start(args, message);
    if (vsnprintf(conf->err.description, sizeof(conf->err.description), message, args) < 0)
    {
        strcpy(conf->err.description, "formatting description failed");
    }
    va_end(args);

    longjmp(conf->err_buf, 1);
}

static void *default_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    if (nsize > 0)
    {
        return realloc(ptr, nsize);
    }
    free(ptr);
    return NULL;
}

static void *push(conf_state *conf, size_t size)
{
    assert(conf != NULL); // LCOV_EXCL_BR_LINE
    return conf->allocfunc(conf->user_data, NULL, 0, size);
}

static void pop(conf_state *conf, void *ptr, size_t size)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(ptr != NULL);
    // LCOV_EXCL_STOP
    conf->allocfunc(conf->user_data, ptr, size, 0);
}

static uchar utf8decode(conf_state *conf, const char *utf8, long *utf8_length)
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
        0, 12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72,  // state 0
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, // state 1
        12, 0, 12, 12, 12, 12, 12, 0, 12, 0, 12, 12,    // state 2
        12, 24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12, // state 3
        12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, // state 4
        12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12, // state 5
        12, 12, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, // state 6
        12, 36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, // state 7
        12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, // state 8
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

    // Transition to the first DFA state.
    uint8_t state = next_UTF8_DFA[byte_to_character_class[bytes[0]]];

    // Consume the remaining UTF-8 bytes.
    for (int i = 1; i < seqlen; i++)
    {
        // Mask off the next byte.
        // It's of the form 10xxxxxx if valid UTF-8.
        value = value << UINT32_C(6) | (uchar)(bytes[i] & UINT8_C(0x3F));

        // Transition to the next DFA state.
        state = next_UTF8_DFA[state + byte_to_character_class[bytes[i]]];
    }

    // Verify the encoded character was well-formed.
    if (state == UINT8_C(0))
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

static bool is_newline(conf_state *conf, const char *string, long *length)
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

static void scan_triple_quoted_literal(conf_state *conf, const char *string, token *tok)
{
    const char *at = string;
    long length;

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

static void scan_single_quoted_literal(conf_state *conf, const char *string, token *tok)
{
    const char *at = string;
    long length;

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

static void scan_literal(conf_state *conf, const char *string, token *tok)
{
    const char *at = string;
    long length;

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

static void scan_whitespace(conf_state *conf, const char *string, token *tok)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(string != NULL);
    assert(tok != NULL);
    // LCOV_EXCL_STOP

    const char *at = string;
    for (;;)
    {
        long length;
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

static void scan_comment(conf_state *conf, const char *string, token *tok)
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

        long length = 0;
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

static void scan_token(conf_state *conf, const char *string, token *tok)
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
        long length;
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

static token_type peek(conf_state *conf, token *tok)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(tok != NULL);
    // LCOV_EXCL_STOP

    if (conf->peek.type == TOK_INVALID)
    {
        for (;;)
        {
            scan_token(conf, conf->needle, &conf->peek);
            if (conf->peek.type == TOK_WHITESPACE)
            {
                conf->needle += conf->peek.lexeme_length;
                continue;
            }
            else if (conf->peek.type == TOK_COMMENT)
            {
                // Prevent the same comment from being reported twice.
                // Comments might be parsed twice when the parser
                // is rewound, but they should only be reported
                // once to the user.
                if (conf->comment_processed <= conf->peek.lexeme)
                {
                    const conf_arg arg = {
                        .lexeme_offset = conf->peek.lexeme,
                        .lexeme_length = conf->peek.lexeme_length,
                    };
                    if (conf->parsefunc(conf->user_data, CONF_COMMENT, 1, &arg) != 0)
                    {
                        die(conf, CONF_USER_ABORTED, conf->needle, "user aborted");
                    }
                    conf->comment_processed = arg.lexeme_offset + arg.lexeme_length;
                }

                conf->needle += conf->peek.lexeme_length;
                continue;
            }
            break;
        }
    }
    *tok = conf->peek;
    return conf->peek.type;
}

static token_type eat(conf_state *conf, token *tok)
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

static long copy_literal(conf_state *conf, char *dest, const token *tok)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(tok != NULL);
    // LCOV_EXCL_STOP

    const char *stop_offset = &conf->string[tok->lexeme + tok->lexeme_length];
    const char *offset = &conf->string[tok->lexeme];
    long nbytes = 0;

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
                long length;
                if (is_newline(conf, offset, &length))
                {
                    offset += length;
                    continue;
                }
            }
        }

        long length = 0;
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

// Parsing commands is a two step process:
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
static void parse_directive(conf_state *conf, int depth)
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

    char *args_buffer = push(conf, buffer_length);
    if (args_buffer == NULL)
    {
        die(conf, CONF_NO_MEMORY, conf->needle, "memory allocation failed");
    }

    const int argc = args_count;
    struct conf_arg *argv = push(conf, argc * sizeof(argv[0]));
    if (argv == NULL)
    {
        pop(conf, args_buffer, buffer_length);
        die(conf, CONF_NO_MEMORY, conf->needle, "memory allocation failed");
    }

    memset(args_buffer, 0, buffer_length);

    char *buffer = args_buffer;
    args_count = 0;
    for (;;)
    {
        peek(conf, &tok);
        if (tok.type == TOK_LITERAL)
        {
            conf_arg *arg = &argv[args_count++];
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

    int r = conf->parsefunc(conf->user_data, CONF_DIRECTIVE, argc, argv);
    pop(conf, argv, argc * sizeof(argv[0]));
    pop(conf, args_buffer, buffer_length);
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

        r = conf->parsefunc(conf->user_data, CONF_SUBDIRECTIVE_PUSH, 0, NULL);
        if (r != 0)
        {
            die(conf, CONF_USER_ABORTED, conf->needle, "user aborted");
        }

        parse_body(conf, depth + 1);

        peek(conf, &tok);
        if (tok.type == '}')
        {
            eat(conf, &tok); // consume '}'

            r = conf->parsefunc(conf->user_data, CONF_SUBDIRECTIVE_POP, 0, NULL);
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

// Command lists are parsed in a single pass and collected into a linked list.
// After parsing is complete and the linked list is fully constructed, then the
// list items are copied to an array for O(1) access.
static void parse_body(conf_state *conf, int depth)
{
    // LCOV_EXCL_START
    assert(conf != NULL);
    assert(depth >= 0);
    // LCOV_EXCL_STOP

    // Parse all subcommands into a linked list.
    for (;;)
    {
        token tok;
        if (peek(conf, &tok) == TOK_EOF)
        {
            break;
        }

        if (tok.type == TOK_LITERAL)
        {
            parse_directive(conf, depth);
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
}

conf_errno conf_parse(const char *string, conf_err *error, void *user_data, conf_parsefunc parsefunc, conf_allocfunc allocfunc)
{
    conf_state state = {0};
    state.string = string;
    state.needle = string;
    state.user_data = user_data;
    state.parsefunc = parsefunc;
    state.allocfunc = allocfunc;
    state.err.where = -1;
    state.err.code = CONF_OK;

    if (state.allocfunc == NULL)
    {
        state.allocfunc = &default_alloc;
    }

    if (setjmp(state.err_buf) != 0)
    {
        if (error != NULL)
        {
            memcpy(error, &state.err, sizeof(error[0]));
        }
        return state.err.code;
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

    if (parsefunc == NULL)
    {
        if (error != NULL)
        {
            error->code = CONF_INVALID_OPERATION;
            strcpy(error->description, "missing function argument");
        }
        return CONF_INVALID_OPERATION;
    }

    // Skip past the a BOM (byte order mark) character if present.
    if (memchr(state.needle, '\0', 3) == NULL)
    {
        if (memcmp(state.needle, "\xEF\xBB\xBF", 3) == 0)
        {
            state.needle += 3;
        }
    }
    parse_body(&state, 0);

    token tok;
    peek(&state, &tok);
    if (tok.type != TOK_EOF)
    {
        assert(tok.type == '}'); // LCOV_EXCL_BR_LINE
        die(&state, CONF_BAD_SYNTAX, state.needle, "found '}' without matching '{'");
    }

    return CONF_OK;
}
