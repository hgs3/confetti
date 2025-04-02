#!/usr/bin/env python3

# Confetti: a configuration language and parser library
# Copyright (c) 2025 Confetti Contributors
#
# This file is part of Confetti, distributed under the MIT License
# For full terms see the included LICENSE file.

from typing import List, Union
from dataclasses import dataclass
from pathlib import Path

@dataclass
class Success:
    value: str

@dataclass
class Error:
    value: str

@dataclass
class TestCase:
    name: str
    input: Union[str,bytes]
    output: Union[Success,Error]

test_cases: List[TestCase] = [
    TestCase(
        "empty",
        # input
        "",
        # output
        Success("")
    ),
    TestCase(
        "empty_with_byte_order_mark",
        # input
        "\uFEFF",
        # output
        Success("")
    ),
    TestCase(
        "empty_with_white_space",
        # input
        "   \t  \n  \u2000 \r \u202F  ",
        # output
        Success("")
    ),
    TestCase(
        "empty_with_white_space_and_byte_order_mark",
        # input
        "\uFEFF   \t  \n  \u2000 \r \u202F  ",
        # output
        Success("")
    ),
    TestCase(
        "lonely_line_continuation",
        # input
        "\\\n",
        # output
        Error("error: unexpected line continuation\n")
    ),
    TestCase(
        "directive_with_byte_order_mark",
        # input
        "\uFEFFfoo",
        # output
        Success("<foo>\n")
    ),
    TestCase(
        "multiples_directives_with_byte_order_mark",
        # input
        "\uFEFFfoo\nbar\nbaz",
        # output
        Success("<foo>\n<bar>\n<baz>\n")
    ),
    TestCase(
        "directive_with_single_argument",
        # input
        "foo",
        # output
        Success("<foo>\n")
    ),
    TestCase(
        "directive_with_multiple_arguments",
        # input
        "foo bar baz",
        # output
        Success("<foo> <bar> <baz>\n")
    ),
    TestCase(
        "directive_with_quoted_argument",
        # input
        "foo \"bar baz\" qux",
        # output
        Success("<foo> <bar baz> <qux>\n")
    ),
    TestCase(
        "backslash_before_last_argument_character",
        # input
        "fooba\\r",
        # output
        Success("<foobar>\n")
    ),
    TestCase(
        "directive_with_single_argument_ending_with_a_backslash",
        # input
        "foo\\\nbar",
        # output
        Error("error: illegal escape character\n")
    ),
    TestCase(
        "directive_with_line_continuation",
        # input
        "foo \\\nbar",
        # output
        Success("<foo> <bar>\n")
    ),
    TestCase(
        "directive_with_multiple_line_continuations",
        # input
        "foo \\\n   \tbar \\\r\nbaz",
        # output
        Success("<foo> <bar> <baz>\n")
    ),
    TestCase(
        "comment_with_line_continuation",
        # input
        "# This comment ends with a line continuation \\\n",
        # output
        Success("")
    ),
    TestCase(
        "directive_with_empty_quoted_argument",
        # input
        '""',
        # output
        Success('<>\n')
    ),
    TestCase(
        "directive_with_closing_quote_escaped",
        # input
        '"foo\\"',
        # output
        Error("error: unclosed quoted\n")
    ),
    TestCase(
        "directive_with_incomplete_escape_sequence_in_quoted_argument",
        # input
        '"foo\\',
        # output
        Error("error: incomplete escape sequence\n")
    ),
    TestCase(
        "directive_with_empty_triple_quoted_argument",
        # input
        '""""""',
        # output
        Success('<>\n')
    ),
    TestCase(
        "lineterm_lf",
        # input
        "foo\nbar",
        # output
        Success("""<foo>
<bar>
""")
    ),
    TestCase(
        "lineterm_vt",
        # input
        "foo\vbar",
        # output
        Success("""<foo>
<bar>
""")
    ),
    TestCase(
        "lineterm_ff",
        # input
        "foo\fbar",
        # output
        Success("""<foo>
<bar>
""")
    ),
    TestCase(
        "lineterm_cr",
        # input
        "foo\rbar",
        # output
        Success("""<foo>
<bar>
""")
    ),
    TestCase(
        "lineterm_crlf",
        # input
        "foo\r\nbar",
        # output
        Success("""<foo>
<bar>
""")
    ),
    TestCase(
        "lineterm_nel",
        # input
        "foo\u0085bar",
        # output
        Success("""<foo>
<bar>
""")
    ),
    TestCase(
        "lineterm_ls",
        # input
        "foo\u2028bar",
        # output
        Success("""<foo>
<bar>
""")
    ),
    TestCase(
        "lineterm_ps",
        # input
        "foo\u2029bar",
        # output
        Success("""<foo>
<bar>
""")
    ),
    TestCase(
        "escape_punctuator",
        # input
        "foo\\{bar",
        # output
        Success("<foo{bar>\n")
    ),
    TestCase(
        "escape_punctuator_starter",
        # input
        "\\{bar\\{",
        # output
        Success("<{bar{>\n")
    ),
    TestCase(
        "escape_punctuator_in_comment",
        # input
        "foo \\#nope\\#\\{ bar",
        # output
        Success("<foo> <#nope#{> <bar>\n")
    ),
    TestCase(
        "escape_punctuator_quoted",
        # input
        '\\"foo\\"\\\'bar\\\'',
        # output
        Success('<"foo"\'bar\'>\n')
    ),
    TestCase(
        "escape_punctuator_all",
        # input
        "foo \\{\\\"\\'\\}\\; bar",
        # output
        Success("<foo> <{\"'};> <bar>\n")
    ),
    TestCase(
        "illegal_escaped_character",
        # input
        'foo\\\x01bar',
        # output
        Error("error: illegal escape character\n")
    ),
    TestCase(
        "term",
        # input
        "foo;bar;baz;",
        # output
        Success("<foo>\n<bar>\n<baz>\n")
    ),
    TestCase(
        "extraneous_term",
        # input
        "foo;;bar",
        # output
        Error("error: unexpected ';'\n")
    ),
    TestCase(
        "term_after_subdirectives",
        # input
        "foo{};bar",
        # output
        Error("error: unexpected ';'\n")
    ),
    TestCase(
        "quoted_term",
        # input
        '"foo ; bar"',
        # output
        Success("<foo ; bar>\n")
    ),
    TestCase(
        "quoted_escape_single_quote",
        # input
        '"foo\\\'bar"',
        # output
        Success("<foo'bar>\n")
    ),
    TestCase(
        "quoted_escape_double_quote",
        # input
        '"foo\\\"bar"',
        # output
        Success("<foo\"bar>\n")
    ),
    TestCase(
        "quoted_escape_hash",
        # input
        '"foo\\#bar"',
        # output
        Success("<foo#bar>\n")
    ),
    TestCase(
        "quoted_escape_semicolon",
        # input
        '"foo\\;bar"',
        # output
        Success("<foo;bar>\n")
    ),
    TestCase(
        "quoted_escape_opening_brace",
        # input
        '"foo\\{bar"',
        # output
        Success("<foo{bar>\n")
    ),
    TestCase(
        "quoted_escape_closing_brace",
        # input
        '"foo\\}bar"',
        # output
        Success("<foo}bar>\n")
    ),
    TestCase(
        "quoted_escape_quoted",
        # input
        '"foo\\bar"',
        # output
        Success("<foobar>\n")
    ),
    TestCase(
        "quoted_escape_slash",
        # input
        '"foo\\\\bar"',
        # output
        Success("<foo\\bar>\n")
    ),
    TestCase(
        "double_quoted_directive_argument",
        # input
        '""a""',
        # output
        Success('<> <a> <>\n')
    ),
    TestCase(
        "quoted_arguments_back_to_back",
        # input
        '"foo""bar"',
        # output
        Success('<foo> <bar>\n')
    ),
    TestCase(
        "missing_closing_quote",
        # input
        '"foo',
        # output
        Error('error: unclosed quoted\n')
    ),
    TestCase(
        "quoted_argument_with_line_continuation",
        # input
        '"foo\\\nbar"',
        # output
        Success("<foobar>\n")
    ),
    TestCase(
        "quoted_argument_with_multiple_line_continuations",
        # input
        '"a\\\nb\\\rc\\\r\nd"',
        # output
        Success("<abcd>\n")
    ),
    TestCase(
        "quoted_argument_with_only_line_continuations",
        # input
        '"\\\n\\\r\\\r\n"',
        # output
        Success("<>\n")
    ),
    TestCase(
        "line_continuation_in_unclosed_quoted_argument",
        # input
        '"foo\\\n',
        # output
        Error("error: unclosed quoted\n")
    ),
    TestCase(
        "triple_quoted_argument_with_erroneous_line_continuation",
        # input
        '"""foo\\\nbar"""',
        # output
        Error("error: incomplete escape sequence\n")
    ),
    TestCase(
        "triple_quoted",
        # input
        '"""foo bar baz"""',
        # output
        Success("<foo bar baz>\n")
    ),
    TestCase(
        "triple_quoted_newline_unclosed",
        # input
        '"""foo bar baz\n',
        # output
        Error("error: unclosed quoted\n")
    ),
    TestCase(
        "triple_quoted_with_nested_single_and_double_quotes",
        # input
        '"""foo " bar "" baz"""',
        # output
        Success('<foo " bar "" baz>\n')
    ),
    TestCase(
        "triple_quoted_argument",
        # input
        'foo """ bar """ baz',
        # output
        Success("<foo> < bar > <baz>\n")
    ),
    TestCase(
        "missing_closing_triple_quotes",
        # input
        '"""missing closing triple quotes',
        # output
        Error("error: unclosed quoted\n")
    ),
    TestCase(
        "escaped_character_in_triple_quoted_argument",
        # input
        '"""foo\\bar"""',
        # output
        Success("<foobar>\n")
    ),
    TestCase(
        "multiple_tripled_quoted_arguments",
        # input
        '"""foo bar""" """baz qux"""',
        # output
        Success("<foo bar> <baz qux>\n")
    ),
    TestCase(
        "triple_quoted_argument_with_first_quote_of_closing_triple_quotes_escaped",
        # input
        '"""foo\\"""',
        # output
        Error("error: unclosed quoted\n")
    ),
    TestCase(
        "triple_quoted_argument_with_illegal_white_space_escape_character",
        # input
        '"""foo \\ bar"""',
        # output
        Error("error: illegal escape character\n")
    ),
    TestCase(
        "illegal_character_in_tripled_quoted_argument",
        # input
        '"""\x01"""',
        # output
        Error("error: illegal character\n")
    ),
    TestCase(
        "illegal_escape_character_in_triple_quoted_argument",
        # input
        '"""foo\\\x01bar"""',
        # output
        Error("error: illegal escape character\n")
    ),
    TestCase(
        "incomplete_escape_sequence_in_triple_quoted_argument",
        # input
        '"""foo\\\nbar"""',
        # output
        Error("error: incomplete escape sequence\n")
    ),
    TestCase(
        "incomplete_escape_sequence_in_quoted_argument",
        # input
        '"""foo\\',
        # output
        Error("error: incomplete escape sequence\n")
    ),
    TestCase(
        "triple_quoted_multi_line",
        # input
        '"""The\nquick\r\nbrown\ffox\u0085jumped\u2028over\u2029the\rlazy dog."""',
        # output
        Success("<The\nquick\r\nbrown\ffox\u0085jumped\u2028over\u2029the\rlazy dog.>\n")
    ),
    TestCase(
        "script_latin",
        # input
        "The quick brown fox jumps over the lazy dog",
        # output
        Success("<The> <quick> <brown> <fox> <jumps> <over> <the> <lazy> <dog>\n")
    ),
    TestCase(
        "script_greek",
        # input
        "Η γρήγορη καφέ αλεπού πηδάει πάνω από το τεμπέλικο σκυλί",
        # output
        Success("<Η> <γρήγορη> <καφέ> <αλεπού> <πηδάει> <πάνω> <από> <το> <τεμπέλικο> <σκυλί>\n")
    ),
    TestCase(
        "script_cyrillic",
        # input
        "Быстрая коричневая лиса прыгает через ленивую собаку",
        # output
        Success("<Быстрая> <коричневая> <лиса> <прыгает> <через> <ленивую> <собаку>\n")
    ),
    TestCase(
        "script_hiragana",
        # input
        "素早い茶色のキツネが怠け者の犬を飛び越えます",
        # output
        Success("<素早い茶色のキツネが怠け者の犬を飛び越えます>\n")
    ),
    TestCase(
        "script_han",
        # input
        "敏捷的棕色狐狸跳过了懒狗",
        # output
        Success("<敏捷的棕色狐狸跳过了懒狗>\n")
    ),
    TestCase(
        "script_hangul",
        # input
        "빠른 갈색 여우는 게으른 개를 뛰어 넘습니다",
        # output
        Success("<빠른> <갈색> <여우는> <게으른> <개를> <뛰어> <넘습니다>\n")
    ),
    TestCase(
        "script_thai",
        # input
        "สุนัขจิ้งจอกสีน้ำตาลเร็วกระโดดข้ามสุนัขขี้เกียจ",
        # output
        Success("<สุนัขจิ้งจอกสีน้ำตาลเร็วกระโดดข้ามสุนัขขี้เกียจ>\n")
    ),
    TestCase(
        "script_hindi",
        # input
        "तेज, भूरी लोमडी आलसी कुत्ते के उपर कूद गई",
        # output
        Success("<तेज,> <भूरी> <लोमडी> <आलसी> <कुत्ते> <के> <उपर> <कूद> <गई>\n")
    ),
    TestCase(
        "script_emoji",
        # input
        "👨🏻‍🚀",
        # output
        Success("<👨🏻‍🚀>\n")
    ),
    TestCase(
        "escape_eof",
        # input
        'foo\\',
        # output
        Error("error: illegal escape character\n")
    ),
    TestCase(
        "line_continuation_before_eof",
        # input
        'foo \\',
        # output
        Error("error: illegal escape character\n")
    ),
    TestCase(
        "line_continuation_to_eof",
        # input
        'foo \\\n',
        # output
        Success("<foo>\n")
    ),
    TestCase(
        "lonely_left_brace",
        # input
        "{",
        # output
        Error("error: unexpected '{'\n")
    ),
    TestCase(
        "lonely_right_brace",
        # input
        "}",
        # output
        Error("error: found '}' without matching '{'\n")
    ),
    TestCase(
        "empty_braces",
        # input
        "x{}",
        # output
        Success("<x>\n")
    ),
    TestCase(
        "subdirectives_begin_after_line_continuation",
        # input
        """foo \\
{ bar }""",
        # output
        Success("""<foo> [
    <bar>
]
""")
    ),
    TestCase(
        "subdirectives_end_after_line_continuation",
        # input
        """foo { bar \\
}""",
        # output
        Success("""<foo> [
    <bar>
]
""")
    ),
    TestCase(
        "empty_braces_multi_line",
        # input
        "x{}y {   } \n"
        "z{\n"
        "\n"
        " }\n",
        # output
        Success("""<x>
<y>
<z>
""")
    ),
    TestCase(
        "one",
        # input
        """foo bar baz
qux

{
    fight club
    movies {
       great pretender

       robin
    }
    are you here
}

scadoodle do
""",
        # output
        Success("""<foo> <bar> <baz>
<qux> [
    <fight> <club>
    <movies> [
        <great> <pretender>
        <robin>
    ]
    <are> <you> <here>
]
<scadoodle> <do>
""")
    ),
    TestCase(
        "two",
        # input
        """foo { bar ; baz } qux
wal do
""",
        # output
        Success("""<foo> [
    <bar>
    <baz>
]
<qux>
<wal> <do>
""")
    ),
    TestCase(
        "markup",
        # input
        """heading "The Raven"
author "Edgar Allan Poe"
paragraph {
  "Once upon a midnight dreary, while I pondered, weak and weary,"
  "Over many a quaint and " bold{"curious volume"} " of forgotten lore-"
}
paragraph {
  "While I nodded, " italic{nearly} bold{napping} ", suddenly there came a tapping,"
  "As of some one gently rapping-rapping at my chamber door."
}
""",
        # output
        Success("""<heading> <The Raven>
<author> <Edgar Allan Poe>
<paragraph> [
    <Once upon a midnight dreary, while I pondered, weak and weary,>
    <Over many a quaint and > <bold> [
        <curious volume>
    ]
    < of forgotten lore->
]
<paragraph> [
    <While I nodded, > <italic> [
        <nearly>
    ]
    <bold> [
        <napping>
    ]
    <, suddenly there came a tapping,>
    <As of some one gently rapping-rapping at my chamber door.>
]
""")
    ),
    TestCase(
        "comment",
        # input
        "# This is a simple comment.",
        # output
        Success("")
    ),
    TestCase(
        "comment_with_illegal_character",
        # input
        "# This comment contains a forbidden character \x01.",
        # output
        Error("error: illegal character\n")
    ),
    TestCase(
        "comment_with_a_malformed_character",
        # input
        b"# Malformed UTF-8: \xF0\x28\x8C\xBC", # Overlong encoded SPACE (U+0020).
        # output
        Error("error: malformed UTF-8\n")
    ),
    TestCase(
        "empty_comment",
        # input
        "#",
        # output
        Success("")
    ),
    TestCase(
        "comment_after_directive",
        # input
        """x # 1 2 3
y # a b c
z
""",
        # output
        Success("""<x>
<y>
<z>
""")
    ),
    TestCase(
        "error_quoted_unterminated",
        # input
        '"foo \n bar"',
        # output
        Error("error: unclosed quoted\n")
    ),
    TestCase(
        "error_quoted_illegal",
        # input
        '"foo \a bar"',
        # output
        Error("error: illegal character\n")
    ),
    TestCase(
        "error_quoted_illegal_space",
        # input
        '"foo \\ bar"',
        # output
        Error("error: illegal escape character\n")
    ),
    TestCase(
        "error_missing_closing_curly_brace",
        # input
        """foo {
    bar

""", # missing closing curly brace
        # output
        Error("error: expected '}'\n")
    ),
    TestCase(
        "error_unexpected_closing_curly_brace",
        # input
        """foo 
    bar
}
""", # unpaired curly brace
        # output
        Error("error: found '}' without matching '{'\n")
    ),
    TestCase(
        "control_z",
        # input
        "foo\u001A",
        # output
        Success("<foo>\n"),
    ),
    TestCase(
        "control_z_unexpected",
        # input
        "fo\u001Ao",
        # output
        Error("error: illegal character U+001A\n")
    ),
    TestCase(
        "control_character",
        # input
        "fo\x01o",
        # output
        Error("error: illegal character U+0001\n")
    ),
    TestCase(
        "unassigned_character",
        # input
        "fo\U000EFFFFo",
        # output
        Error("error: illegal character U+EFFFF\n")
    ),
    TestCase(
        "lonely_high_surrogate_character",
        # input
        b"fo\xD8\x3Do",
        # output
        Error("error: malformed UTF-8\n")
    ),
    TestCase(
        "lonely_low_surrogate_character",
        # input
        b"fo\xDE\x00o",
        # output
        Error("error: malformed UTF-8\n")
    ),
    TestCase(
        "truncated_character",
        # input
        b"\xF0\x9F\x98", # Truncated Grinning Face (U+1F600)
        # output
        Error("error: malformed UTF-8\n")
    ),
    TestCase(
        "truncated_overlong_character_sequence",
        # input
        b"\xC1", # Truncated overlong encoded sequence.
        # output
        Error("error: malformed UTF-8\n")
    ),
    TestCase(
        "overlong_character_sequence",
        # input
        b"\xC0\xA0", # Overlong encoded SPACE (U+0020).
        # output
        Error("error: malformed UTF-8\n")
    ),
    TestCase(
        "invalid_octet_sequence",
        # input
        b"\xF0\x28\x8C\xBC", # Overlong encoded SPACE (U+0020).
        # output
        Error("error: malformed UTF-8\n")
    ),
    TestCase(
        "invalid_octet_sequence_in_directive",
        # input
        b"foo\xF0\x28\x8C\xBCbar", # Overlong encoded SPACE (U+0020).
        # output
        Error("error: malformed UTF-8\n")
    ),
    TestCase(
        "private_use_character",
        # input
        "fo\U0010FFFDo",
        # output
        Success("<fo\U0010FFFDo>\n")
    ),
    TestCase(
        "general_category_cased_letter_in_argument",
        # input
        "\u0041\u0061\u01C5", # Lu Ll Lt
        # output
        Success("<\u0041\u0061\u01C5>\n")
    ),
    TestCase(
        "general_category_letter_in_argument",
        # input
        "\u02B0\u00AA", # Lm Lo
        # output
        Success("<\u02B0\u00AA>\n")
    ),
    TestCase(
        "general_category_mark_in_argument",
        # input
        "\u0300\u0903\u0488", # Mn Mc Me
        # output
        Success("<\u0300\u0903\u0488>\n")
    ),
    TestCase(
        "general_category_number_in_argument",
        # input
        "\u0030\u16EE\u00B2", # Nd Nl No
        # output
        Success("<\u0030\u16EE\u00B2>\n")
    ),
    TestCase(
        "general_category_punctuation_in_argument",
        # input
        "\u005F\u002D\u0028\u0029\u00AB\u00BB\u0021", # Pc Pd Ps Pe Pi Pf Po
        # output
        Success("<\u005F\u002D\u0028\u0029\u00AB\u00BB\u0021>\n")
    ),
    TestCase(
        "general_category_symbol_in_argument",
        # input
        "\u002B\u0024\u005E\u00A6", # Sm Sc Sk So
        # output
        Success("<\u002B\u0024\u005E\u00A6>\n")
    ),
    TestCase(
        "general_category_other_in_argument",
        # input
        "\u00AD\uE000", # Cf Co
        # output
        Success("<\u00AD\uE000>\n")
    ),
    TestCase(
        "general_category_cased_letter_in_quoted_argument",
        # input
        '"\u0041\u0061\u01C5"', # Lu Ll Lt
        # output
        Success("<\u0041\u0061\u01C5>\n")
    ),
    TestCase(
        "general_category_letter_in_quoted_argument",
        # input
        '"\u02B0\u00AA"', # Lm Lo
        # output
        Success("<\u02B0\u00AA>\n")
    ),
    TestCase(
        "general_category_mark_in_quoted_argument",
        # input
        '"\u0300\u0903\u0488"', # Mn Mc Me
        # output
        Success("<\u0300\u0903\u0488>\n")
    ),
    TestCase(
        "general_category_number_in_quoted_argument",
        # input
        '"\u0030\u16EE\u00B2"', # Nd Nl No
        # output
        Success("<\u0030\u16EE\u00B2>\n")
    ),
    TestCase(
        "general_category_punctuation_in_quoted_argument",
        # input
        '"\u005F\u002D\u0028\u0029\u00AB\u00BB\u0021"', # Pc Pd Ps Pe Pi Pf Po
        # output
        Success("<\u005F\u002D\u0028\u0029\u00AB\u00BB\u0021>\n")
    ),
    TestCase(
        "general_category_symbol_in_quoted_argument",
        # input
        '"\u002B\u0024\u005E\u00A6"', # Sm Sc Sk So
        # output
        Success("<\u002B\u0024\u005E\u00A6>\n")
    ),
    TestCase(
        "general_category_other_in_quoted_argument",
        # input
        '"\u00AD\uE000"', # Cf Co
        # output
        Success("<\u00AD\uE000>\n")
    ),
    TestCase(
        "general_category_cased_letter_in_triple_quoted_argument",
        # input
        '"""\u0041\u0061\u01C5"""', # Lu Ll Lt
        # output
        Success("<\u0041\u0061\u01C5>\n")
    ),
    TestCase(
        "general_category_letter_in_triple_quoted_argument",
        # input
        '"""\u02B0\u00AA"""', # Lm Lo
        # output
        Success("<\u02B0\u00AA>\n")
    ),
    TestCase(
        "general_category_mark_in_triple_quoted_argument",
        # input
        '"""\u0300\u0903\u0488"""', # Mn Mc Me
        # output
        Success("<\u0300\u0903\u0488>\n")
    ),
    TestCase(
        "general_category_number_in_triple_quoted_argument",
        # input
        '"""\u0030\u16EE\u00B2"""', # Nd Nl No
        # output
        Success("<\u0030\u16EE\u00B2>\n")
    ),
    TestCase(
        "general_category_punctuation_in_triple_quoted_argument",
        # input
        '"""\u005F\u002D\u0028\u0029\u00AB\u00BB\u0021"""', # Pc Pd Ps Pe Pi Pf Po
        # output
        Success("<\u005F\u002D\u0028\u0029\u00AB\u00BB\u0021>\n")
    ),
    TestCase(
        "general_category_symbol_in_triple_quoted_argument",
        # input
        '"""\u002B\u0024\u005E\u00A6"""', # Sm Sc Sk So
        # output
        Success("<\u002B\u0024\u005E\u00A6>\n")
    ),
    TestCase(
        "general_category_other_in_triple_quoted_argument",
        # input
        '"""\u00AD\uE000"""', # Cf Co
        # output
        Success("<\u00AD\uE000>\n")
    ),
    TestCase(
        "general_category_cased_letter_in_comment",
        # input
        "#\u0041\u0061\u01C5", # Lu Ll Lt
        # output
        Success("")
    ),
    TestCase(
        "general_category_letter_in_comment",
        # input
        "#\u02B0\u00AA", # Lm Lo
        # output
        Success("")
    ),
    TestCase(
        "general_category_mark_in_comment",
        # input
        "#\u0300\u0903\u0488", # Mn Mc Me
        # output
        Success("")
    ),
    TestCase(
        "general_category_number_in_comment",
        # input
        "#\u0030\u16EE\u00B2", # Nd Nl No
        # output
        Success("")
    ),
    TestCase(
        "general_category_punctuation_in_comment",
        # input
        "#\u005F\u002D\u0028\u0029\u00AB\u00BB\u0021", # Pc Pd Ps Pe Pi Pf Po
        # output
        Success("")
    ),
    TestCase(
        "general_category_symbol_in_comment",
        # input
        "#\u002B\u0024\u005E\u00A6", # Sm Sc Sk So
        # output
        Success("")
    ),
    TestCase(
        "general_category_other_in_comment",
        # input
        "#\u00AD\uE000", # Cf Co
        # output
        Success("")
    ),
    # This is the "kichen sink" example from the Confetii website.
    TestCase(
        "kitchen_sink",
        # input
        """# This is a comment.

probe-device eth0 eth1

user * {
    login anonymous
    password "${ENV:ANONPASS}"
    machine 167.89.14.1
    proxy {
        try-ports 582 583 584
    }
}

user "Joe Williams" {
    login joe
    machine 167.89.14.1
}""",
        # output
        Success("""<probe-device> <eth0> <eth1>
<user> <*> [
    <login> <anonymous>
    <password> <${ENV:ANONPASS}>
    <machine> <167.89.14.1>
    <proxy> [
        <try-ports> <582> <583> <584>
    ]
]
<user> <Joe Williams> [
    <login> <joe>
    <machine> <167.89.14.1>
]
""")
    ),
    TestCase(
        "user_settings",
        # input
        """username JohnDoe
language en-US
theme dark
notifications on
""",
        # output
        Success("""<username> <JohnDoe>
<language> <en-US>
<theme> <dark>
<notifications> <on>
""")
    ),
    TestCase(
        "application_settings",
        # input
        """application {
    version 1.2.3
    auto-update true
    log-level debug
}

display {
    resolution 1920x1080
    full-screen true
}
""",
        # output
        Success("""<application> [
    <version> <1.2.3>
    <auto-update> <true>
    <log-level> <debug>
]
<display> [
    <resolution> <1920x1080>
    <full-screen> <true>
]
"""),
    ),
    TestCase(
        "document_markup",
        # input
        """chapter "The Raven"
author "Edgar Allan Poe"
section "First Act" {
  paragraph {
    "Once upon a midnight dreary, while I pondered, weak and weary,"
    "Over many a quaint and " bold{"curious"} " volume of forgotten lore-\"
  }
  paragraph {
    "While I nodded, nearly napping, suddenly there came a tapping,"
    "As of some one " italic{"gently"} " rapping-rapping at my chamber door."
  }
}
""",
        # output
        Success("""<chapter> <The Raven>
<author> <Edgar Allan Poe>
<section> <First Act> [
    <paragraph> [
        <Once upon a midnight dreary, while I pondered, weak and weary,>
        <Over many a quaint and > <bold> [
            <curious>
        ]
        < volume of forgotten lore->
    ]
    <paragraph> [
        <While I nodded, nearly napping, suddenly there came a tapping,>
        <As of some one > <italic> [
            <gently>
        ]
        < rapping-rapping at my chamber door.>
    ]
]
""")
    ),
    TestCase(
        "workflow_automation",
        # input
        """build {
    description "Compile the source code"
    command "gcc -o program source.c"
}

clean {
    description "Clean the build directory"
    command "rm -rf build/"
}

test {
    description "Run unit tests"
    command "./tests/run.sh"
    depends_on { build }
}""",
        # output
        Success("""<build> [
    <description> <Compile the source code>
    <command> <gcc -o program source.c>
]
<clean> [
    <description> <Clean the build directory>
    <command> <rm -rf build/>
]
<test> [
    <description> <Run unit tests>
    <command> <./tests/run.sh>
    <depends_on> [
        <build>
    ]
]
""")
    ),
    TestCase(
        "user_interface",
        # input
        '''Application {
    VerticalLayout {
        Label {
            text "This application has a single button."
        }

        Button {
            text "Click Me"
            on_click """
function() {
    console.log(`You clicked a button named: ${this.text}`);
}
"""
        }
    }
}
''',
        # output
        Success('''<Application> [
    <VerticalLayout> [
        <Label> [
            <text> <This application has a single button.>
        ]
        <Button> [
            <text> <Click Me>
            <on_click> <
function() {
    console.log(`You clicked a button named: ${this.text}`);
}
>
        ]
    ]
]
''')
    ),
    TestCase(
        "ait_training",
        # input
        """model {
    type "neural_network"
    architecture {
      layers {
        layer { type input; size 784 }
        layer { type dense; units 128; activation "relu" }
        layer { type output; units 10; activation "softmax" }
      }
  }

  training {
    data "/path/to/training/data"
    epochs 20
    early_stopping on
  }
}
""",
        # output
        Success("""<model> [
    <type> <neural_network>
    <architecture> [
        <layers> [
            <layer> [
                <type> <input>
                <size> <784>
            ]
            <layer> [
                <type> <dense>
                <units> <128>
                <activation> <relu>
            ]
            <layer> [
                <type> <output>
                <units> <10>
                <activation> <softmax>
            ]
        ]
    ]
    <training> [
        <data> </path/to/training/data>
        <epochs> <20>
        <early_stopping> <on>
    ]
]
""")
    ),
    TestCase(
        "material_definitions",
        # input
        """material water
    opacity 0.5
    pass
        diffuse materials/liquids/water.png
    pass
        diffuse materials/liquids/water2.png
        blend-mode additive
""",
        # output
        Success("""<material> <water>
<opacity> <0.5>
<pass>
<diffuse> <materials/liquids/water.png>
<pass>
<diffuse> <materials/liquids/water2.png>
<blend-mode> <additive>
"""),
    ),
    TestCase(
        "stack_based_language",
        # input
        """push 1
push 2
add     # Pop the top two numbers and push their sum.
pop $x  # Pop the sum and store it in $x.
print "1 + 2 ="
print $x
""",
        # output
        Success("""<push> <1>
<push> <2>
<add>
<pop> <$x>
<print> <1 + 2 =>
<print> <$x>
""")
    ),
    TestCase(
        "control_flow",
        # input
        """set $retry-count to 3
for $i in $retry-count {
    if $is_admin {
        print "Access granted"
        send_email "admin@example.com"
        exit 0 # Success!
    }
}
exit 1 # Failed to confirm admin role.
""",
        # output
        Success("""<set> <$retry-count> <to> <3>
<for> <$i> <in> <$retry-count> [
    <if> <$is_admin> [
        <print> <Access granted>
        <send_email> <admin@example.com>
        <exit> <0>
    ]
]
<exit> <1>
""")
    ),
    TestCase(
        "state_machine",
        # input
        """states {
    greet_player {
        look_at $player
        wait 1s # Pause one second before walking towards the player.
        walk_to $player
        say "Good evening traveler."
    }

    last_words {
        say "Tis a cruel world!"
    }
}

events {
    player_spotted {
        goto_state greet_player
    }

    died {
        goto_state last_words
    }
}
""",
        Success("""<states> [
    <greet_player> [
        <look_at> <$player>
        <wait> <1s>
        <walk_to> <$player>
        <say> <Good evening traveler.>
    ]
    <last_words> [
        <say> <Tis a cruel world!>
    ]
]
<events> [
    <player_spotted> [
        <goto_state> <greet_player>
    ]
    <died> [
        <goto_state> <last_words>
    ]
]
""")
    )
]

longest_input = 0
longest_output = 0

for tcase in test_cases:
    if isinstance(tcase.input, str):
        bytes = tcase.input.encode("utf-8")
    else:
        bytes = tcase.input

    # Generate fuzz testing corpus.
    Path(f"corpus").mkdir(exist_ok=True)
    with open(f"corpus/{tcase.name}.conf", "wb") as out:
        out.write(bytes)

    # Generate test data for 3rd party implementations to validate against.
    Path(f"suite").mkdir(exist_ok=True)
    with open(f"suite/{tcase.name}.in", "wb") as out:
        out.write(bytes)
    if isinstance(tcase.output, Success):
        with open(f"suite/{tcase.name}.out", "wb") as out:
            out.write(tcase.output.value.encode("utf-8"))
    else:
        with open(f"suite/{tcase.name}.err", "wb") as out:
            out.write(tcase.output.value.encode("utf-8"))

    longest_input = max(longest_input, len(bytes))
    longest_output = max(longest_output, len(tcase.output.value))

# Generate a C header with the test data for the reference implementation.
with open(f"test_suite.h", "w", encoding="utf-8", newline="\n") as out:
    out.write("struct TestData {\n")
    out.write("    const char *name;\n")
    out.write(f"    unsigned char input[{longest_input+1}];\n")
    out.write(f"    unsigned char output[{longest_output+1}];\n")
    out.write("};\n\n")
    out.write("static const struct TestData tests_utf8[] = {\n")
    for tcase in test_cases:
        if isinstance(tcase.input, str):
            bytes = tcase.input.encode("utf-8")
        else:
            bytes = tcase.input
        out.write(f'    {{ "{tcase.name}", ')
        out.write("{")
        for b in bytes:
            out.write("0x{:02X},".format(int(b)))
        out.write("0x00}, {")
        for b in tcase.output.value.encode("utf-8"):
            out.write("0x{:02X},".format(int(b)))
        out.write("0x00} },\n")
    out.write("};\n")
    out.write("\n")
