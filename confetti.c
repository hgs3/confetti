/*
 * Confetti: a configuration language and parser library
 * Copyright (c) 2025 Henry G. Stratmann III
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License.
 * For full terms see the included LICENSE file.
 */

// This implementation of Confetti is more complex than it ought to be because:
//
//   (1) It supports two API's - a callback-based tree-walking API as well as an
//       API for building an in-memory tree structure.
//
//   (2) It supports all optional extensions specified in the annex of the Confetti 
//       language specification. These extensions are opt-in via the public API.
//
//   (3) It tracks meta-data about the source text, like code comments, as well as
//       additional toggles in the public API, like for rejecting source text with
//       bidirectional formatting characters.
//
//   (4) Lastly, it is designed for efficiency, rather than optimal clarity.
//
// Purpose-built implementations can omit unused extensions, ignore comments, and
// simplify the API. Ideally, a more straightforward implementation could,
// potentially, be only a few hundred lines of C, at most.

#include "confetti.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <setjmp.h>
#include <assert.h>
#include <stdint.h>
#include <stdalign.h>
#include <stddef.h>

// When gathering branch coverage, do not let untaken assert branches contribute negatively to
// the metrics. Asserts are never supposed to fail so their branches will not be taken.
#if defined(CODE_COVERAGE)
#undef assert
#define assert(x)
#endif

// This is a workaround for Visual Studio's lack of support for max_align_t.
#if defined(_MSC_VER)
#define max_align_t 16
#endif

typedef uint32_t uchar; // Unicode scalar value.

uint8_t conf_uniflags(uint32_t cp);

#define IS_FORBIDDEN_CHARACTER 0x1 // set of forbidden characters
#define IS_SPACE_CHARACTER 0x2 // set of white space and new line characters
#define IS_PUNCTUATOR_CHARACTER 0x4 // set of reserved punctuator characters
#define IS_ARGUMENT_CHARACTER 0x8 // set of characters that are valid in an unquoted argument
#define IS_BIDI_CHARACTER 0x10 // set of bidirectional formatting characters
#define IS_ESCAPABLE_CHARACTER (IS_ARGUMENT_CHARACTER | IS_PUNCTUATOR_CHARACTER)

#define BAD_ENCODING 0x110000

typedef enum token_type
{
    TOK_INVALID,
    TOK_EOF,
    TOK_COMMENT,
    TOK_WHITESPACE,
    TOK_NEWLINE,
    TOK_ARGUMENT,
    TOK_CONTINUATION,
    TOK_SEMICOLON = ';',
    TOK_LCURLYB = '{',
    TOK_RCURLYB = '}',
} token_type;

typedef enum token_flags
{
    CONF_QUOTED = 0x1,
    CONF_TRIPLE_QUOTED = 0x2,
    CONF_EXPRESSION = 0x4,
} token_flags;

typedef struct token
{
    size_t lexeme;
    size_t lexeme_length;

    // Number of bytes to trim from the beginning and end of the lexeme when it's converted
    // to a value. For example, when a quoted argument is processed, the enclsoing quotes
    // are trimmed.
    size_t trim;

    token_type type;
    token_flags flags;
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

// Represents a set of punctuator arguments beginning with the same Unicode scalar value.
struct punctset
{
    size_t size; // The size, in bytes, of this structure in memory.
    long length; // The total number of punctuators sharing the same starting character.
    char punctuators[]; // List of punctuator strings delimited by a zero byte.
};

struct conf_unit
{
    const char *string; // Points to the beginning of the string being parsed.
    const char *needle; // Points to the current location being parsed.
    
    conf_walkfn walk;
    token peek; // Current, but processed token.

    // The punctuator starters array is an array of Unicode scalar values where each scalar
    // is a unique starting character amongst the set of punctuators. For example, if we
    // have the punctuator set {'+', '+=', '-', '-='}, then this arrays length is two
    // because we have two unique starting characters: '+' and '-'.
    uchar *punctuator_starters;
    size_t punctuator_starters_size;

    // The punctuators array is an array of arrays, where each subarray contains punctuators
    // that begin with the same Unicode scalar value (i.e. the same starter character).
    struct punctset **punctuators;
    long punctuators_count;

    // Comments are tracked in a linked list when the source text is parsed, but then
    // they are moved to an array for O(1) access time after parsing completes.
    long comments_count;
    struct comment **comments;
    struct comment *comment_head;
    struct comment *comment_tail;
    size_t comment_processed;

    // These are user-provided structures.
    conf_options options;
    conf_extensions extensions;

    conf_directive *root;

    jmp_buf err_buf;
    conf_error err;

    alignas(max_align_t) unsigned char padding[sizeof(conf_directive)];
};

static void parse_body(conf_unit *conf, conf_directive *parent, int depth);

_Noreturn static void die(conf_unit *conf, conf_errno error, const char *where, const char *message, ...)
{
    assert(conf != NULL);
    assert(where != NULL);

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
    assert(n < (int)sizeof(conf->err.description));

    longjmp(conf->err_buf, 1);
}

static void *default_alloc(void *ud, void *ptr, size_t size)
{
    assert(size > 0);
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

static void *new(conf_unit *conf, size_t size)
{
    assert(conf != NULL);
    assert(size > 0);
    return conf->options.allocator(conf->options.user_data, NULL, size);
}

static void *zero_new(conf_unit *conf, size_t size)
{
    void *ptr = new(conf, size);
    if (ptr != NULL)
    {
        (void)memset(ptr, 0, size);
    }
    return ptr;
}

static void delete(conf_unit *conf, void *ptr, size_t size)
{
    assert(conf != NULL);
    assert(ptr != NULL);
    assert(size > 0);
    conf->options.allocator(conf->options.user_data, ptr, size);
}

static uchar utf8decode2(const char *utf8, size_t *utf8_length)
{
    assert(utf8 != NULL);

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
        return BAD_ENCODING;
    }

    // Check if the character ends prematurely due to a null terminator.
    for (int i = 1; i < seqlen; i++)
    {
        if (bytes[i] == '\0')
        {
            return BAD_ENCODING;
        }
    }

    // Consume the first UTF-8 byte.
    uchar value = (uchar)(bytes[0] & bytes_needed_for_UTF8_sequence[256 + seqlen]);

    // Transition to the first DFA state.
    uint8_t unit = next_UTF8_DFA[byte_to_character_class[bytes[0]]];

    // Consume the remaining UTF-8 bytes.
    for (int i = 1; i < seqlen; i++)
    {
        // Mask off the next byte.
        // It's of the form 10xxxxxx if valid UTF-8.
        value = value << UINT32_C(6) | (uchar)(bytes[i] & UINT8_C(0x3F));

        // Transition to the next DFA state.
        unit = next_UTF8_DFA[unit + byte_to_character_class[bytes[i]]];
    }

    // Verify the encoded character was well-formed.
    if (unit == UINT8_C(0))
    {
        if (utf8_length != NULL)
        {
            *utf8_length = seqlen;
        }
        return value;
    }
    
    return BAD_ENCODING;
}

static uchar utf8decode(conf_unit *conf, const char *utf8, size_t *utf8_length)
{
    const uchar scalar = utf8decode2(utf8, utf8_length);
    if (scalar == BAD_ENCODING)
    {
        die(conf, CONF_ILLEGAL_BYTE_SEQUENCE, utf8, "malformed UTF-8");
    }
    return scalar;
}

static bool is_newline(conf_unit *conf, const char *string, size_t *length)
{
    assert(conf != NULL);
    assert(string != NULL);
    assert(length != NULL);

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

// Scan expression arguments is implemented using a "virtual" stack data structure.
// When '(' is encountered, it's pushed, when ')' is encountered, it's popped.
// When the stack is empty, the expression has been fully processed.
static void scan_expression_argument(conf_unit *conf, const char *string, token *tok)
{
    assert(conf != NULL);
    assert(string != NULL);
    assert(string[0] == '(');
    assert(tok != NULL);

    const char *at = string + 1; // +1 to skip the opening '(' character
    size_t stack = 1; // "push" an opening '(' character onto the "stack"

    for (;;)
    {
        if (at[0] == '\0')
        {
            die(conf, CONF_BAD_SYNTAX, string, "incomplete expression");
        }
        
        if (at[0] == '(')
        {
            stack += 1; // "push" a '(' character onto the stack
            at += 1;
        }
        else if (at[0] == ')')
        {
            stack -= 1; // "pop" a ')' character from the stack
            at += 1;

            // If the "stack" is empty, then the complete expression has been processed.
            if (stack == 0)
            {
                break;
            }
        }
        else
        {
            size_t length = 0;
            const uchar cp = utf8decode(conf, at, &length);

            if (conf_uniflags(cp) & IS_FORBIDDEN_CHARACTER)
            {
                die(conf, CONF_BAD_SYNTAX, at, "illegal character");
            }

            if ((conf_uniflags(cp) & IS_BIDI_CHARACTER) && !conf->options.allow_bidi)
            {
                die(conf, CONF_BAD_SYNTAX, at, "illegal bidirectional character");
            }

            at += length;
        }
    }

    tok->lexeme = string - conf->string;
    tok->lexeme_length = at - string;
    tok->type = TOK_ARGUMENT;
    tok->flags = CONF_EXPRESSION;
    tok->trim = 1;
}

static void scan_triple_quoted_argument(conf_unit *conf, const char *string, token *tok)
{
    const char *at = string;
    size_t length;

    assert(conf != NULL);
    assert(string != NULL);
    assert(tok != NULL);
    assert((at[0] == '"') && (at[1] == '"') && (at[2] == '"'));

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

        if ((conf_uniflags(cp) & IS_BIDI_CHARACTER) && !conf->options.allow_bidi)
        {
            die(conf, CONF_BAD_SYNTAX, at, "illegal bidirectional character");
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

            if ((conf_uniflags(cp) & IS_BIDI_CHARACTER) && !conf->options.allow_bidi)
            {
                die(conf, CONF_BAD_SYNTAX, at, "illegal bidirectional character");
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
    tok->type = TOK_ARGUMENT;
    tok->flags = CONF_TRIPLE_QUOTED;
    tok->trim = 3;
}

static void scan_single_quoted_argument(conf_unit *conf, const char *string, token *tok)
{
    const char *at = string;
    size_t length;

    assert(conf != NULL);
    assert(string != NULL);
    assert(tok != NULL);
    assert(at[0] == '"');

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

            if ((conf_uniflags(cp) & IS_BIDI_CHARACTER) && !conf->options.allow_bidi)
            {
                die(conf, CONF_BAD_SYNTAX, at, "illegal bidirectional character");
            }
        }
        else
        {
            if ((conf_uniflags(cp) & (IS_ESCAPABLE_CHARACTER | IS_SPACE_CHARACTER)) == 0)
            {
                die(conf, CONF_BAD_SYNTAX, at, "illegal character");
            }

            if ((conf_uniflags(cp) & IS_BIDI_CHARACTER) && !conf->options.allow_bidi)
            {
                die(conf, CONF_BAD_SYNTAX, at, "illegal bidirectional character");
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
    tok->type = TOK_ARGUMENT;
    tok->flags = CONF_QUOTED;
    tok->trim = 1;
}

static bool scan_punctuator_argument(conf_unit *conf, const char *string, token *tok, uchar starter)
{
    assert(conf != NULL);
    assert(conf->punctuators_count > 0);
    assert(string != NULL);
    assert(tok != NULL);

    size_t longest_match = 0;
    for (long i = 0; i < conf->punctuators_count; i++)
    {
        if (conf->punctuator_starters[i] != starter)
        {
            continue;
        }

        size_t buffer_offset = 0;
        const struct punctset *set = conf->punctuators[i];
        for (long i = 0; i < set->length; i++)
        {
            const char *punctuator = &set->punctuators[buffer_offset];
            const size_t punctuator_length = strlen(punctuator);
            if (punctuator_length >= longest_match)
            {
                if (strncmp(string, punctuator, punctuator_length) == 0)
                {
                    tok->lexeme = string - conf->string;
                    tok->lexeme_length =  punctuator_length;
                    tok->type = TOK_ARGUMENT;
                    tok->flags = 0;
                    tok->trim = 0;
                    longest_match = punctuator_length;    
                }
            }
            buffer_offset += punctuator_length + 1;
        }
    }

    if (longest_match > 0)
    {
        return true;
    }
    return false;
}

static void scan_argument(conf_unit *conf, const char *string, token *tok)
{
    const char *at = string;
    size_t length;

    assert(conf != NULL);
    assert(string != NULL);
    assert(tok != NULL);
    
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

            if ((conf_uniflags(cp) & IS_BIDI_CHARACTER) && !conf->options.allow_bidi)
            {
                die(conf, CONF_BAD_SYNTAX, at, "illegal bidirectional character");
            }

            at += length;
            continue;
        }

        if ((conf_uniflags(cp) & IS_ARGUMENT_CHARACTER) == 0)
        {
            break;
        }

        if ((conf_uniflags(cp) & IS_BIDI_CHARACTER) && !conf->options.allow_bidi)
        {
            die(conf, CONF_BAD_SYNTAX, at, "illegal bidirectional character");
        }

        // If the expression arguments extension is enabled, then do
        // not consider it part of this argument.
        if (conf->extensions.expression_arguments && cp == '(')
        {
            break;
        }

        // If the punctuator arguments extension is enabled, then check if
        // the current character is the start of one. If so, then do not
        // interpret it as part of this extension argument.
        if (conf->punctuators_count > 0)
        {
            if (scan_punctuator_argument(conf, at, tok, cp))
            {
                break;
            }
        }

        at += length;
    }

    tok->lexeme = string - conf->string;
    tok->lexeme_length = at - string;
    tok->type = TOK_ARGUMENT;
    tok->flags = 0;
    tok->trim = 0;
}

static void scan_whitespace(conf_unit *conf, const char *string, token *tok)
{
    assert(conf != NULL);
    assert(string != NULL);
    assert(tok != NULL);

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
    tok->trim = 0;
}

static void scan_single_line_comment(conf_unit *conf, const char *string, token *tok)
{
    assert(conf != NULL);
    assert(string != NULL);
    assert(string[0] == '#' || (string[0] == '/' && string[1] == '/'));
    assert(tok != NULL);

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
        if (conf_uniflags(cp) & IS_FORBIDDEN_CHARACTER)
        {
            die(conf, CONF_BAD_SYNTAX, at, "illegal character");
        }

        if ((conf_uniflags(cp) & IS_BIDI_CHARACTER) && !conf->options.allow_bidi)
        {
            die(conf, CONF_BAD_SYNTAX, at, "illegal bidirectional character");
        }

        at += length;
    }

    tok->lexeme = string - conf->string;
    tok->lexeme_length = at - string;
    tok->type = TOK_COMMENT;
    tok->flags = 0;
    tok->trim = 0;
}

static void scan_multi_line_comment(conf_unit *conf, const char *string, token *tok)
{
    assert(conf != NULL);
    assert(string != NULL);
    assert(string[0] == '/' && string[1] == '*');
    assert(tok != NULL);

    const char *at = string;
    for (;;)
    {
        if (*at == '\0')
        {
            die(conf, CONF_BAD_SYNTAX, string, "unterminated multi-line comment");
        }

        if (at[0] == '*' && at[1] == '/')
        {
            at += 2;
            break;
        }

        size_t length = 0;
        const uchar cp = utf8decode(conf, at, &length);
        if (conf_uniflags(cp) & IS_FORBIDDEN_CHARACTER)
        {
            die(conf, CONF_BAD_SYNTAX, at, "illegal character");
        }

        if ((conf_uniflags(cp) & IS_BIDI_CHARACTER) && !conf->options.allow_bidi)
        {
            die(conf, CONF_BAD_SYNTAX, at, "illegal bidirectional character");
        }

        at += length;
    }

    tok->lexeme = string - conf->string;
    tok->lexeme_length = at - string;
    tok->type = TOK_COMMENT;
    tok->flags = 0;
    tok->trim = 0;
}

static void scan_token(conf_unit *conf, const char *string, token *tok)
{
    assert(conf != NULL);
    assert(string != NULL);
    assert(tok != NULL);

    if (string[0] == '#')
    {
        scan_single_line_comment(conf, string, tok);
        return;
    }

    if (conf->extensions.c_style_comments)
    {
        // Check for a C style single line comment, e.g. "// this is a commment"
        if (string[0] == '/' && string[1] == '/')
        {
            scan_single_line_comment(conf, string, tok);
            return;
        }

        // Check for a C style multi-line comment, e.g. "/* this is a commment */"
        if (string[0] == '/' && string[1] == '*')
        {
            scan_multi_line_comment(conf, string, tok);
            return;
        }
    }

    if (is_newline(conf, string, &tok->lexeme_length))
    {
        tok->type = TOK_NEWLINE;
        tok->lexeme = string - conf->string;
        tok->flags = 0;
        tok->trim = 0;
        return;
    }

    const uchar cp = utf8decode(conf, string, NULL);
    if (conf_uniflags(cp) & IS_SPACE_CHARACTER)
    {
        scan_whitespace(conf, string, tok);
        return;
    }

    if ((conf_uniflags(cp) & IS_BIDI_CHARACTER) && !conf->options.allow_bidi)
    {
        die(conf, CONF_BAD_SYNTAX, string, "illegal bidirectional character");
    }

    if (conf->punctuators_count > 0)
    {
        if (scan_punctuator_argument(conf, string, tok, cp))
        {
            return;
        }
    }

    if (conf->extensions.expression_arguments)
    {
        if (string[0] == '(')
        {
            scan_expression_argument(conf, string, tok);
            return;
        }
    }

    if ((string[0] == '{') || (string[0] == '}'))
    {
        tok->type = (token_type)string[0];
        tok->lexeme = string - conf->string;
        tok->lexeme_length = 1;
        tok->flags = 0;
        tok->trim = 0;
        return;
    }
    
    if (string[0] == '"')
    {
        if ((string[1] == '"') && (string[2] == '"'))
        {
            scan_triple_quoted_argument(conf, string, tok);
        }
        else
        {
            scan_single_quoted_argument(conf, string, tok);
        }
        return;
    }

    if (string[0] == ';')
    {
        tok->type = TOK_SEMICOLON;
        tok->lexeme = string - conf->string;
        tok->lexeme_length = 1;
        tok->flags = 0;
        tok->trim = 0;
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
            tok->trim = 0;
            return;
        }
    }

    if (conf_uniflags(cp) & IS_ARGUMENT_CHARACTER)
    {
        scan_argument(conf, string, tok);
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
        tok->trim = 0;
        return;
    }

    if (cp == 0x0)
    {
        tok->type = TOK_EOF;
        tok->lexeme = string - conf->string;
        tok->lexeme_length = 0;
        tok->flags = 0;
        tok->trim = 0;
        return;
    }
    
    die(conf, CONF_BAD_SYNTAX, string, "illegal character U+%04X", cp);
}

static void record_comment(conf_unit *unit, const conf_comment *data)
{
    struct comment *comment = new(unit, sizeof(comment[0]));
    if (comment == NULL)
    {
        die(unit, CONF_OUT_OF_MEMORY, unit->needle, "memory allocation failed");
    }
    comment->data.offset = data->offset;
    comment->data.length = data->length;
    comment->next = NULL;

    if (unit->comment_head == NULL)
    {
        assert(unit->comment_tail == NULL);
        unit->comment_head = comment;
        unit->comment_tail = comment;
    }
    else
    {
        assert(unit->comment_tail != NULL);
        unit->comment_tail->next = comment;
        unit->comment_tail = comment;
    }
    unit->comments_count += 1;
}

static token_type peek(conf_unit *unit, token *tok)
{
    assert(unit != NULL);
    assert(tok != NULL);

    if (unit->peek.type == TOK_INVALID)
    {
        for (;;)
        {
            scan_token(unit, unit->needle, &unit->peek);
            if (unit->peek.type == TOK_WHITESPACE)
            {
                unit->needle += unit->peek.lexeme_length;
                continue;
            }
            else if (unit->peek.type == TOK_COMMENT)
            {
                // Prevent the same comment from being reported twice.
                // Comments might be parsed twice when the parser
                // is rewound, but they should only be reported
                // once to the user.
                if (unit->comment_processed <= unit->peek.lexeme)
                {
                    const conf_comment comment = {
                        .offset = unit->peek.lexeme,
                        .length = unit->peek.lexeme_length,
                    };
                    if (unit->walk == NULL)
                    {
                        record_comment(unit, &comment);
                    }
                    else if (unit->walk(unit->options.user_data, CONF_COMMENT, 0, NULL, &comment) != 0)
                    {
                        die(unit, CONF_USER_ABORTED, unit->needle, "user aborted");
                    }
                    unit->comment_processed = comment.offset + comment.length;
                }
                unit->needle += unit->peek.lexeme_length;
                continue;
            }
            break;
        }
    }
    *tok = unit->peek;
    return unit->peek.type;
}

static token_type eat(conf_unit *conf, token *tok)
{
    assert(conf != NULL);
    assert(tok != NULL);

    peek(conf, tok);
    conf->needle += tok->lexeme_length;
    conf->peek.type = TOK_INVALID;
    return tok->type;
}

static size_t copy_token_to_buffer(conf_unit *conf, char *dest, const token *tok)
{
    assert(conf != NULL);
    assert(tok != NULL);

    const char *stop_offset = &conf->string[tok->lexeme + tok->lexeme_length];
    const char *offset = &conf->string[tok->lexeme];
    size_t nbytes = 0;

    // Discard the N surrounding characters (e.g. quotes in a quoted literal).
    offset += tok->trim;
    stop_offset -= tok->trim;

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

static void parse_directive(conf_unit *conf, conf_directive *parent, int depth)
{
    assert(conf != NULL);
    assert(depth >= 0);

    token tok;

    // (1) figure out how much memory is needed for the arguments

    const token saved_peek = conf->peek; // save parser unit
    const char *saved_needle = conf->needle;

    long argument_count = 0;
    long buffer_length = 0;
    for (;;)
    {
        peek(conf, &tok);
        if (tok.type == TOK_ARGUMENT)
        {
            argument_count += 1;
            buffer_length += copy_token_to_buffer(conf, NULL, &tok) + 1; // +1 for null byte
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
    conf->peek = saved_peek; // rewind parser unit
    conf->needle = saved_needle;

    // (2) allocate storage for the arguments and copy the data to it

    const size_t size = sizeof(conf_directive) + (size_t)buffer_length;
    conf_directive *dir = zero_new(conf, size);
    if (dir == NULL)
    {
        die(conf, CONF_OUT_OF_MEMORY, conf->needle, "memory allocation failed");
    }
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
        if (tok.type == TOK_ARGUMENT)
        {
            conf_argument *arg = &argv[argument_count++];
            arg->lexeme_offset = tok.lexeme;
            arg->lexeme_length = tok.lexeme_length;
            arg->value = buffer;
            arg->is_expression = (tok.flags & CONF_EXPRESSION) ? true : false;
            buffer += copy_token_to_buffer(conf, buffer, &tok) + 1; // +1 for null byte
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
        assert(parent->subdir_tail == NULL);
        parent->subdir_head = dir;
        parent->subdir_tail = dir;
    }
    else
    {
        assert(parent->subdir_tail != NULL);
        parent->subdir_tail->next = dir;
        parent->subdir_tail = dir;
    }
    
    // Check for an optional, terminating semicolon.
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

    // Check for an optional subdirective.
    if (tok.type == '{')
    {
        eat(conf, &tok); // consume '{'
        parse_body(conf, dir, depth + 1);

        peek(conf, &tok);
        if (tok.type == '}')
        {
            eat(conf, &tok); // consume '}'
            peek(conf, &tok);
        }
        else
        {
            die(conf, CONF_BAD_SYNTAX, conf->needle, "expected '}'");
        }

        // Check for an optional, terminating semicolon.
        if (tok.type == ';')
        {
            eat(conf, &tok); // consume ';'
        }
    }
}

static void walk_directive(conf_unit *conf, int depth)
{
    assert(conf != NULL);
    assert(depth >= 0);

    token tok;

    // (1) figure out how much memory is needed for the arguments

    const token saved_peek = conf->peek; // save parser state
    const char *saved_needle = conf->needle;

    int args_count = 0;
    int buffer_length = 0;
    for (;;)
    {
        peek(conf, &tok);
        if (tok.type == TOK_ARGUMENT)
        {
            args_count += 1;
            buffer_length += copy_token_to_buffer(conf, NULL, &tok) + 1; // +1 for null byte
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

    char *args_buffer = zero_new(conf, buffer_length);
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

    char *buffer = args_buffer;
    args_count = 0;
    for (;;)
    {
        peek(conf, &tok);
        if (tok.type == TOK_ARGUMENT)
        {
            conf_argument *arg = &argv[args_count++];
            arg->lexeme_offset = tok.lexeme;
            arg->lexeme_length = tok.lexeme_length;
            arg->value = buffer;
            arg->is_expression = (tok.flags & CONF_EXPRESSION) ? true : false;
            buffer += copy_token_to_buffer(conf, buffer, &tok) + 1; // +1 for null byte
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

    // Check for an optional, terminating semicolon.
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

    // Check for an optional subdirective.
    if (tok.type == '{')
    {
        eat(conf, &tok); // consume '{'

        r = conf->walk(conf->options.user_data, CONF_BLOCK_ENTER, 0, NULL, NULL);
        if (r != 0)
        {
            die(conf, CONF_USER_ABORTED, conf->needle, "user aborted");
        }

        parse_body(conf, NULL, depth + 1);

        peek(conf, &tok);
        if (tok.type == '}')
        {
            eat(conf, &tok); // consume '}'
            peek(conf, &tok);

            r = conf->walk(conf->options.user_data, CONF_BLOCK_LEAVE, 0, NULL, NULL);
            if (r != 0)
            {
                die(conf, CONF_USER_ABORTED, conf->needle, "user aborted");
            }
        }
        else
        {
            die(conf, CONF_BAD_SYNTAX, conf->needle, "expected '}'");
        }

        // Check for a terminating semicolon.
        if (tok.type == ';')
        {
            eat(conf, &tok); // consume ';'
        }
    }
}

// Directive lists are parsed in a single pass and collected into a linked list.
// After parsing is complete and the linked list is fully constructed, then the
// list items are copied to an array for O(1) access.
static void parse_body(conf_unit *conf, conf_directive *parent, int depth)
{
    assert(conf != NULL);
    assert(depth >= 0);

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

        if (tok.type == TOK_ARGUMENT)
        {
            if (parent == NULL)
            {
                walk_directive(conf, depth);
                assert(conf->walk != NULL);
            }
            else
            {
                parse_directive(conf, parent, depth);
                subdirs_count += 1;
                assert(conf->walk == NULL);
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

        assert((tok.type == ';') || (tok.type == '{'));
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

const conf_directive *conf_get_directive(const conf_directive *dir, long index)
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

long conf_get_directive_count(const conf_directive *dir)
{
    if (dir == NULL)
    {
        return 0;
    }
    return dir->subdir_count;
}

const conf_directive *conf_get_root(const conf_unit *unit)
{
    if (unit == NULL)
    {
        return NULL;
    }
    return unit->root;
}

const conf_argument *conf_get_argument(const conf_directive *dir, long index)
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

long conf_get_argument_count(const conf_directive *dir)
{
    if (dir == NULL)
    {
        return 0;
    }
    return dir->arguments_count;
}

const conf_comment *conf_get_comment(const conf_unit *unit, long index)
{
    if (unit == NULL)
    {
        return NULL;
    }
    if (index < 0 || index >= unit->comments_count)
    {
        return NULL;
    }
    return &unit->comments[index]->data;
}

long conf_get_comment_count(const conf_unit *unit)
{
    if (unit == NULL)
    {
        return 0;
    }
    return unit->comments_count;
}

static void free_directive(conf_unit *conf, conf_directive *dir)
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
    
    assert(dir->arguments_count > 0);
    delete(conf, dir->arguments, sizeof(dir->arguments[0]) * dir->arguments_count);
    delete(conf, dir, sizeof(dir[0]) + dir->buffer_length);
}

void deinit_configuration_unit(conf_unit *unit)
{
    assert(unit != NULL);

    if (unit->root != NULL)
    {
        conf_directive *root = unit->root;
        conf_directive *dir = root->subdir_head;
        while (dir != NULL)
        {
            conf_directive *next = dir->next;
            free_directive(unit, dir);
            dir = next;
        }

        if (root->subdir_count > 0)
        {
            delete(unit, root->subdir, sizeof(root->subdir[0]) * root->subdir_count);
        }
    }

    if (unit->comments_count > 0)
    {
        struct comment *comment = unit->comment_head;
        while (comment != NULL)
        {
            struct comment *next = comment->next;
            delete(unit, comment, sizeof(comment[0]));
            comment = next;
        }

        if (unit->comments != NULL)
        {
            delete(unit, unit->comments, sizeof(unit->comments[0]) * unit->comments_count);
        }
    }

    if (unit->punctuator_starters != NULL)
    {
        delete(unit, unit->punctuator_starters, unit->punctuator_starters_size);
    }

    if (unit->punctuators != NULL)
    {
        for (long i = 0; i < unit->punctuators_count; i++)
        {
            struct punctset *punct = unit->punctuators[i];
            if (punct != NULL)
            {
                delete(unit, punct, punct->size);
            }
        }
        delete(unit, unit->punctuators, sizeof(unit->punctuators[0]) * unit->punctuators_count);
    }
}

void conf_free(conf_unit *unit)
{
    if (unit != NULL)
    {
        deinit_configuration_unit(unit);
        delete(unit, unit, sizeof(unit[0]));
    }
}

static void parse_configuration_unit(conf_unit *unit)
{
    // Skip past the a BOM (byte order mark) character if present.
    if (memchr(unit->needle, '\0', 3) == NULL)
    {
        if (memcmp(unit->needle, "\xEF\xBB\xBF", 3) == 0)
        {
            unit->needle += 3;
        }
    }

    // Parse the Confetti configuration unit.
    parse_body(unit, unit->root, 0);

    // Verify the configuration unit ended by checking for extraneous tokens.
    token tok;
    peek(unit, &tok);
    if (tok.type != TOK_EOF)
    {
        assert(tok.type == '}');
        die(unit, CONF_BAD_SYNTAX, unit->needle, "found '}' without matching '{'");
    }
}

static conf_errno init_punctuator_arguments(conf_unit *unit, const char **punctuator_arguments)
{
    struct counters
    {
        long unique_strings;
        long buffer_length;
        long buffer_offset;
    };

    uchar *starters = NULL;
    struct counters *counters = NULL;

    // Count how many punctuator arguments there are.
    long count = 0;
    size_t index = 0;
    for (;;)
    {
        const char *string = punctuator_arguments[index];
        if (string == NULL)
        {
            break;
        }
        index += 1;

        if (string[0] == '\0')
        {
            continue;
        }
        count += 1;

        // Verify the string only contains valid argument characters; it
        // cannot contain white space, reserved, or forbidden characters.
        for (;;)
        {
            size_t byte_count = 0;
            const uchar cp = utf8decode2(string, &byte_count);
            if (cp == '\0')
            {
                break;
            }
            
            if (cp == BAD_ENCODING)
            {
                unit->err.code = CONF_ILLEGAL_BYTE_SEQUENCE;
                strcpy(unit->err.description, "punctuator argument with malformed UTF-8");
                goto cleanup;
            }

            // If the expression arguments extension is enabled, then disallow parentheses
            // as they reserved characters with the extension.
            if (unit->extensions.expression_arguments)
            {
                if (cp == '(' || cp == ')')
                {
                    unit->err.code = CONF_INVALID_OPERATION;
                    strcpy(unit->err.description, "illegal punctuator argument character");
                    goto cleanup;
                }
            }

            if ((conf_uniflags(cp) & IS_ARGUMENT_CHARACTER) == 0)
            {
                unit->err.code = CONF_INVALID_OPERATION;
                strcpy(unit->err.description, "illegal punctuator argument character");
                goto cleanup;
            }

            string += byte_count;
        }
    }

    // If the array is empty, then there are no punctuators to add.
    if (count == 0)
    {
        return CONF_NO_ERROR;
    }

    // Create an array large enough to accommodate the unique starting character of each punctuator.
    // The array might not be fully used if multiple punctuators begin with the same character.
    starters = zero_new(unit, sizeof(starters[0]) * count);
    if (starters == NULL)
    {
        unit->err.code = CONF_OUT_OF_MEMORY;
        strcpy(unit->err.description, "memory allocation failed");
        goto cleanup;
    }
    unit->punctuator_starters = starters;
    unit->punctuator_starters_size = sizeof(starters[0]) * count;

    // Create a temporary array for counters.
    counters = zero_new(unit, sizeof(counters[0]) * count);
    if (counters == NULL)
    {
        unit->err.code = CONF_OUT_OF_MEMORY;
        strcpy(unit->err.description, "memory allocation failed");
        goto cleanup;
    }

    // Count all unique starter characters.
    size_t unique_starters = 0;
    index = 0;
    for (;;)
    {
        const char *string = punctuator_arguments[index];
        if (string == NULL)
        {
            break;
        }
        index += 1;

        const size_t length = strlen(string);
        if (length == 0)
        {
            continue;
        }

        const uchar cp = utf8decode2(string, NULL);
        struct counters *counter = counters;
        uchar *starter = starters;
        bool added = false;

        // Check if this starting character is already in the starter set.
        // This is an O(1) operation, but the assumption is (1) there won't be too many custom punctuators
        // in real world applications and (2) the memory layout of this structure is fairly compact in memory.
        for (size_t i = 0; i < unique_starters; i++, starter++, counter++)
        {
            if ((*starter) == cp)
            {
                counter->buffer_length += length + 1;
                counter->unique_strings += 1;
                added = true;
                break;
            }
        }

        // If this starter hasn't been registered yet, then register it.
        if (!added)
        {
            (*starter) = cp;
            counter->buffer_length = length + 1;
            counter->unique_strings = 1;
            unique_starters += 1;
        }
    }
    assert(unique_starters > 0);

    // Allocate an array large enough to accomidate pointers to each string for each unique starter.
    struct punctset **punctuators = zero_new(unit, sizeof(punctuators[0]) * unique_starters);
    if (punctuators == NULL)
    {
        unit->err.code = CONF_OUT_OF_MEMORY;
        strcpy(unit->err.description, "memory allocation failed");
        goto cleanup;
    }
    unit->punctuators = punctuators;
    unit->punctuators_count = unique_starters;

    // Begin copying the punctuator data to a cache-friendly buffer.
    index = 0;
    for (;;)
    {
        const char *string = punctuator_arguments[index];
        if (string == NULL)
        {
            break;
        }
        index += 1;

        const size_t length = strlen(string);
        if (length == 0)
        {
            continue;
        }

        const uchar cp = utf8decode2(string, NULL);
        struct punctset *punct = NULL;
        struct counters *counter = NULL;

        // Check if this starting character is already in the starter set.
        // This is an O(1) operation, but the assumption is (1) there won't be too many custom punctuators
        // in real world applications and (2) the memory layout of this structure is fairly compact in memory.
        for (size_t i = 0; i < unique_starters; i++) // LCOV_EXCL_BR_LINE: The number of starters is always non-zero.
        {
            if (starters[i] == cp)
            {
                if (punctuators[i] == NULL)
                {
                    counter = &counters[i];
                    const size_t size = sizeof(punct[0]) + sizeof(punct[0].punctuators[0]) * counter->buffer_length;
                    punct = zero_new(unit, size);
                    if (punct == NULL)
                    {
                        unit->err.code = CONF_OUT_OF_MEMORY;
                        strcpy(unit->err.description, "memory allocation failed");
                        goto cleanup;
                    }
                    punct->length = counter->unique_strings;
                    punct->size = size;
                    punctuators[i] = punct;
                }
                else
                {
                    counter = &counters[i];
                    punct = punctuators[i];
                }
                break;
            }
        }
        assert(punct != NULL);
        assert(counter != NULL);

        // Copy the punctuator to the cache-friendly buffer.
        memcpy(&punct->punctuators[counter->buffer_offset], string, length + 1);
        counter->buffer_offset += length + 1;
    }

cleanup:
    if (counters != NULL)
    {
        delete(unit, counters, sizeof(counters[0]) * count);
    }
    return unit->err.code;
}

// Initializes a Confetti configuration unit structure. This initilaization is common to both the walk() and parse() interfaces.
static conf_errno init_configuration_unit(conf_unit *unit, const char *string, const conf_options *options, conf_error *error, conf_walkfn walk)
{
    memset(unit, 0, sizeof(unit[0]));
    unit->string = string;
    unit->needle = string;
    unit->err.where = 0;
    unit->err.code = CONF_NO_ERROR;
    unit->walk = walk;

    if (options != NULL)
    {
        if (options->extensions != NULL)
        {
            unit->extensions = *options->extensions;
        }
        unit->options = *options;
    }

    if (unit->options.max_depth < 1)
    {
        unit->options.max_depth = 20; // Default maximum nesting depth.
    }

    if (unit->options.allocator == NULL)
    {
        unit->options.allocator = &default_alloc;
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

    if (unit->extensions.punctuator_arguments)
    {
        if (init_punctuator_arguments(unit, unit->extensions.punctuator_arguments) != CONF_NO_ERROR)
        {
            if (error != NULL)
            {
                memcpy(error, &unit->err, sizeof(unit->err));
            }
            return unit->err.code;
        }
    }

    return CONF_NO_ERROR;
}

conf_unit *conf_parse(const char *string, const conf_options *options, conf_error *error)
{
    conf_unit *unit = NULL, tmp;
    const conf_errno eno = init_configuration_unit(&tmp, string, options, error, NULL);
    if (eno != CONF_NO_ERROR)
    {
        deinit_configuration_unit(&tmp);
        return NULL;
    }

    // Allocate the top-level directive and then begin parsing.
    unit = new(&tmp, sizeof(tmp));
    if (unit == NULL)
    {
        if (error != NULL)
        {
            error->code = CONF_OUT_OF_MEMORY;
            strcpy(error->description, "memory allocation failed");
        }
        deinit_configuration_unit(&tmp);
        return NULL;
    }
    memcpy(unit, &tmp, sizeof(unit[0]));
    unit->root = (conf_directive *)unit->padding;

    // Setup exception-like handling for unrecoverable errors.
    if (setjmp(unit->err_buf) != 0)
    {
        if (error != NULL)
        {
            memcpy(error, &unit->err, sizeof(error[0]));
        }
        conf_free(unit);
        return NULL;
    }
    parse_configuration_unit(unit);

    // Convert the comments linked list to an array for O(1) access.
    if (unit->comments_count > 0)
    {
        struct comment **comments = new(unit, sizeof(comments[0]) * unit->comments_count);
        if (comments == NULL)
        {
            die(unit, CONF_OUT_OF_MEMORY, unit->needle, "memory allocation failed");
        }

        // Copy subdirective pointers to the array.
        long index = 0;
        for (struct comment *curr = unit->comment_head; curr != NULL; curr = curr->next)
        {
            comments[index] = curr;
            index += 1;
        }
        unit->comments = comments;
    }

    if (error != NULL)
    {
        error->where = unit->needle - unit->string;
        error->code = CONF_NO_ERROR;
        strcpy(error->description, "no error");
    }
    return unit;
}

conf_errno conf_walk(const char *string, const conf_options *options, conf_error *error, conf_walkfn walk)
{
    // The configuration unit walker interface requires a callback function to invoke
    // when an "interesting" configuration unit element is found, e.g. a directive.
    if (walk == NULL)
    {
        if (error != NULL)
        {
            error->code = CONF_INVALID_OPERATION;
            strcpy(error->description, "missing function argument");
        }
        return CONF_INVALID_OPERATION;
    }

    conf_unit unit;
    const conf_errno eno = init_configuration_unit(&unit, string, options, error, walk);
    if (eno != CONF_NO_ERROR)
    {
        deinit_configuration_unit(&unit);
        return eno;
    }

    // Setup exception-like handling for unrecoverable errors.
    if (setjmp(unit.err_buf) == 0)
    {
        parse_configuration_unit(&unit);
        if (error != NULL)
        {
            error->where = unit.needle - unit.string;
            error->code = CONF_NO_ERROR;
            strcpy(error->description, "no error");
        }
    }
    else if (error != NULL)
    {
        memcpy(error, &unit.err, sizeof(error[0]));
    }

    deinit_configuration_unit(&unit);
    return unit.err.code;
}
