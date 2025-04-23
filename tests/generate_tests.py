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
class Extension:
    pass

@dataclass
class CommentExtension(Extension):
    pass

@dataclass
class ExpressionArgumentsExtension(Extension):
    pass

@dataclass
class PunctuatorArgumentsExtension(Extension):
    punctuators: List[str]

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
    extensions: List[Extension]

test_cases: List[TestCase] = [
    TestCase(
        "empty",
        # input
        "",
        # output
        Success(""),
        # extensions
        []
    ),
    TestCase(
        "empty_with_byte_order_mark",
        # input
        "\uFEFF",
        # output
        Success(""),
        # extensions
        []
    ),
    TestCase(
        "empty_with_white_space",
        # input
        "   \t  \n  \u2000 \r \u202F  ",
        # output
        Success(""),
        # extensions
        []
    ),
    TestCase(
        "empty_with_white_space_and_byte_order_mark",
        # input
        "\uFEFF   \t  \n  \u2000 \r \u202F  ",
        # output
        Success(""),
        # extensions
        []
    ),
    TestCase(
        "lonely_line_continuation",
        # input
        "\\\n",
        # output
        Error("error: unexpected line continuation\n"),
        # extensions
        []
    ),
    TestCase(
        "directive_with_byte_order_mark",
        # input
        "\uFEFFfoo",
        # output
        Success("<foo>\n"),
        # extensions
        []
    ),
    TestCase(
        "multiples_directives_with_byte_order_mark",
        # input
        "\uFEFFfoo\nbar\nbaz",
        # output
        Success("<foo>\n<bar>\n<baz>\n"),
        # extensions
        []
    ),
    TestCase(
        "directive_with_single_argument",
        # input
        "foo",
        # output
        Success("<foo>\n"),
        # extensions
        []
    ),
    TestCase(
        "directive_with_multiple_arguments",
        # input
        "foo bar baz",
        # output
        Success("<foo> <bar> <baz>\n"),
        # extensions
        []
    ),
    TestCase(
        "directive_with_quoted_argument",
        # input
        """foo "bar baz" qux""",
        # output
        Success("<foo> <bar baz> <qux>\n"),
        # extensions
        []
    ),
    TestCase(
        "directive_with_escaped_quoted_argument",
        # input
        """foo \\"bar baz\\" qux""",
        # output
        Success("""<foo> <"bar> <baz"> <qux>\n"""),
        # extensions
        []
    ),
    TestCase(
        "backslash_before_last_argument_character",
        # input
        "fooba\\r",
        # output
        Success("<foobar>\n"),
        # extensions
        []
    ),
    TestCase(
        "directive_with_single_argument_ending_with_a_backslash",
        # input
        "foo\\\nbar",
        # output
        Error("error: illegal escape character\n"),
        # extensions
        []
    ),
    TestCase(
        "directive_with_line_continuation",
        # input
        "foo \\\nbar",
        # output
        Success("<foo> <bar>\n"),
        # extensions
        []
    ),
    TestCase(
        "directive_with_multiple_line_continuations",
        # input
        "foo \\\n   \tbar \\\r\nbaz",
        # output
        Success("<foo> <bar> <baz>\n"),
        # extensions
        []
    ),
    TestCase(
        "comment_with_line_continuation",
        # input
        "# This comment ends with a line continuation \\\n",
        # output
        Success(""),
        # extensions
        []
    ),
    TestCase(
        "directive_with_empty_quoted_argument",
        # input
        '""',
        # output
        Success('<>\n'),
        # extensions
        []
    ),
    TestCase(
        "directive_with_closing_quote_escaped",
        # input
        '"foo\\"',
        # output
        Error("error: unclosed quoted\n"),
        # extensions
        []
    ),
    TestCase(
        "directive_with_incomplete_escape_sequence_in_quoted_argument",
        # input
        '"foo\\',
        # output
        Error("error: incomplete escape sequence\n"),
        # extensions
        []
    ),
    TestCase(
        "directive_with_empty_triple_quoted_argument",
        # input
        '""""""',
        # output
        Success('<>\n'),
        # extensions
        []
    ),
    TestCase(
        "lineterm_lf",
        # input
        "foo\nbar",
        # output
        Success("""<foo>
<bar>
"""),
        # extensions
        []
    ),
    TestCase(
        "lineterm_vt",
        # input
        "foo\vbar",
        # output
        Success("""<foo>
<bar>
"""),
        # extensions
        []
    ),
    TestCase(
        "lineterm_ff",
        # input
        "foo\fbar",
        # output
        Success("""<foo>
<bar>
"""),
        # extensions
        []
    ),
    TestCase(
        "lineterm_cr",
        # input
        "foo\rbar",
        # output
        Success("""<foo>
<bar>
"""),
        # extensions
        []
    ),
    TestCase(
        "lineterm_crlf",
        # input
        "foo\r\nbar",
        # output
        Success("""<foo>
<bar>
"""),
        # extensions
        []
    ),
    TestCase(
        "lineterm_nel",
        # input
        "foo\u0085bar",
        # output
        Success("""<foo>
<bar>
"""),
        # extensions
        []
    ),
    TestCase(
        "lineterm_ls",
        # input
        "foo\u2028bar",
        # output
        Success("""<foo>
<bar>
"""),
        # extensions
        []
    ),
    TestCase(
        "lineterm_ps",
        # input
        "foo\u2029bar",
        # output
        Success("""<foo>
<bar>
"""),
        # extensions
        []
    ),
    TestCase(
        "escape_punctuator",
        # input
        "foo\\{bar",
        # output
        Success("<foo{bar>\n"),
        # extensions
        []
    ),
    TestCase(
        "escape_punctuator_starter",
        # input
        "\\{bar\\{",
        # output
        Success("<{bar{>\n"),
        # extensions
        []
    ),
    TestCase(
        "escape_punctuator_in_comment",
        # input
        "foo \\#nope\\#\\{ bar",
        # output
        Success("<foo> <#nope#{> <bar>\n"),
        # extensions
        []
    ),
    TestCase(
        "escape_punctuator_quoted",
        # input
        '\\"foo\\"\\\'bar\\\'',
        # output
        Success('<"foo"\'bar\'>\n'),
        # extensions
        []
    ),
    TestCase(
        "escape_punctuator_all",
        # input
        "foo \\{\\\"\\'\\}\\; bar",
        # output
        Success("<foo> <{\"'};> <bar>\n"),
        # extensions
        []
    ),
    TestCase(
        "illegal_escaped_character",
        # input
        'foo\\\x01bar',
        # output
        Error("error: illegal escape character\n"),
        # extensions
        []
    ),
    TestCase(
        "term",
        # input
        "foo;bar;baz;",
        # output
        Success("<foo>\n<bar>\n<baz>\n"),
        # extensions
        []
    ),
    TestCase(
        "term_after_subdirectives",
        # input
        "foo { bar } ; baz",
        # output
        Success("""<foo> [
    <bar>
]
<baz>
"""),
        # extensions
        []
    ),
        TestCase(
        "term_after_multi_line_subdirectives",
        # input
        """foo
{
    bar
};
baz""",
        # output
        Success("""<foo> [
    <bar>
]
<baz>
"""),
        # extensions
        []
    ),
    TestCase(
        "term_after_subdirectives_twice",
        # input
        "foo { bar } ; baz { qux } ;",
        # output
        Success("""<foo> [
    <bar>
]
<baz> [
    <qux>
]
"""),
        # extensions
        []
    ),
    TestCase(
        "term_after_empty_subdirectives",
        # input
        "foo{};bar",
        # output
        Success("""<foo>
<bar>
"""),
        # extensions
        []
    ),
    TestCase(
        "extraneous_term",
        # input
        "foo;;bar",
        # output
        Error("error: unexpected ';'\n"),
        # extensions
        []
    ),
    TestCase(
        "extraneous_term_after_newline",
        # input
        """foo
; bar
""",
        # output
        Error("error: unexpected ';'\n"),
        # extensions
        []
    ),
    TestCase(
        "extraneous_term_after_subdirectives",
        # input
        "foo { bar } ;; baz",
        # output
        Error("error: unexpected ';'\n"),
        # extensions
        []
    ),
    TestCase(
        "extraneous_term_before_subdirective",
        # input
        "foo ; { bar } baz",
        # output
        Error("error: unexpected '{'\n"),
        # extensions
        []
    ),
    TestCase(
        "extraneous_term_after_newline_before_subdirectives",
        # input
        """foo
; { bar }""",
        # output
        Error("error: unexpected ';'\n"),
        # extensions
        []
    ),
    TestCase(
        "extraneous_term_after_subdirectives_multi_line",
        # input
        """foo { bar }
; baz""",
        # output
        Error("error: unexpected ';'\n"),
        # extensions
        []
    ),
    TestCase(
        "extraneous_term_after_multi_line_subdirectives",
        # input
        """foo
{
    bar
}
;
baz""",
        # output
        Error("error: unexpected ';'\n"),
        # extensions
        []
    ),
    TestCase(
        "quoted_term",
        # input
        '"foo ; bar"',
        # output
        Success("<foo ; bar>\n"),
        # extensions
        []
    ),
    TestCase(
        "quoted_escape_single_quote",
        # input
        '"foo\\\'bar"',
        # output
        Success("<foo'bar>\n"),
        # extensions
        []
    ),
    TestCase(
        "quoted_escape_double_quote",
        # input
        '"foo\\\"bar"',
        # output
        Success("<foo\"bar>\n"),
        # extensions
        []
    ),
    TestCase(
        "quoted_escape_hash",
        # input
        '"foo\\#bar"',
        # output
        Success("<foo#bar>\n"),
        # extensions
        []
    ),
    TestCase(
        "quoted_escape_semicolon",
        # input
        '"foo\\;bar"',
        # output
        Success("<foo;bar>\n"),
        # extensions
        []
    ),
    TestCase(
        "quoted_escape_opening_brace",
        # input
        '"foo\\{bar"',
        # output
        Success("<foo{bar>\n"),
        # extensions
        []
    ),
    TestCase(
        "quoted_escape_closing_brace",
        # input
        '"foo\\}bar"',
        # output
        Success("<foo}bar>\n"),
        # extensions
        []
    ),
    TestCase(
        "quoted_escape_quoted",
        # input
        '"foo\\bar"',
        # output
        Success("<foobar>\n"),
        # extensions
        []
    ),
    TestCase(
        "quoted_escape_slash",
        # input
        '"foo\\\\bar"',
        # output
        Success("<foo\\bar>\n"),
        # extensions
        []
    ),
    TestCase(
        "double_quoted_directive_argument",
        # input
        '""a""',
        # output
        Success('<> <a> <>\n'),
        # extensions
        []
    ),
    TestCase(
        "quoted_arguments_back_to_back",
        # input
        '"foo""bar"',
        # output
        Success('<foo> <bar>\n'),
        # extensions
        []
    ),
    TestCase(
        "quoted_argument_without_closing_quote",
        # input
        'foo"bar',
        # output
        Error('error: unclosed quoted\n'),
        # extensions
        []
    ),
    TestCase(
        "missing_closing_quote",
        # input
        '"foo',
        # output
        Error('error: unclosed quoted\n'),
        # extensions
        []
    ),
    TestCase(
        "quoted_argument_with_line_continuation",
        # input
        '"foo\\\nbar"',
        # output
        Success("<foobar>\n"),
        # extensions
        []
    ),
    TestCase(
        "quoted_argument_with_multiple_line_continuations",
        # input
        '"a\\\nb\\\rc\\\r\nd"',
        # output
        Success("<abcd>\n"),
        # extensions
        []
    ),
    TestCase(
        "quoted_argument_with_only_line_continuations",
        # input
        '"\\\n\\\r\\\r\n"',
        # output
        Success("<>\n"),
        # extensions
        []
    ),
    TestCase(
        "line_continuation_in_unclosed_quoted_argument",
        # input
        '"foo\\\n',
        # output
        Error("error: unclosed quoted\n"),
        # extensions
        []
    ),
    TestCase(
        "triple_quoted_argument_with_erroneous_line_continuation",
        # input
        '"""foo\\\nbar"""',
        # output
        Error("error: incomplete escape sequence\n"),
        # extensions
        []
    ),
    TestCase(
        "triple_quoted",
        # input
        '"""foo bar baz"""',
        # output
        Success("<foo bar baz>\n"),
        # extensions
        []
    ),
    TestCase(
        "triple_quoted_newline_unclosed",
        # input
        '"""foo bar baz\n',
        # output
        Error("error: unclosed quoted\n"),
        # extensions
        []
    ),
    TestCase(
        "triple_quoted_with_nested_single_and_double_quotes",
        # input
        '"""foo " bar "" baz"""',
        # output
        Success('<foo " bar "" baz>\n'),
        # extensions
        []
    ),
    TestCase(
        "triple_quoted_argument",
        # input
        'foo """ bar """ baz',
        # output
        Success("<foo> < bar > <baz>\n"),
        # extensions
        []
    ),
    TestCase(
        "missing_closing_triple_quotes",
        # input
        '"""missing closing triple quotes',
        # output
        Error("error: unclosed quoted\n"),
        # extensions
        []
    ),
    TestCase(
        "escaped_character_in_triple_quoted_argument",
        # input
        '"""foo\\bar"""',
        # output
        Success("<foobar>\n"),
        # extensions
        []
    ),
    TestCase(
        "multiple_tripled_quoted_arguments",
        # input
        '"""foo bar""" """baz qux"""',
        # output
        Success("<foo bar> <baz qux>\n"),
        # extensions
        []
    ),
    TestCase(
        "triple_quoted_argument_with_first_quote_of_closing_triple_quotes_escaped",
        # input
        '"""foo\\"""',
        # output
        Error("error: unclosed quoted\n"),
        # extensions
        []
    ),
    TestCase(
        "triple_quoted_argument_with_illegal_white_space_escape_character",
        # input
        '"""foo \\ bar"""',
        # output
        Error("error: illegal escape character\n"),
        # extensions
        []
    ),
    TestCase(
        "illegal_character_in_tripled_quoted_argument",
        # input
        '"""\x01"""',
        # output
        Error("error: illegal character\n"),
        # extensions
        []
    ),
    TestCase(
        "illegal_escape_character_in_triple_quoted_argument",
        # input
        '"""foo\\\x01bar"""',
        # output
        Error("error: illegal escape character\n"),
        # extensions
        []
    ),
    TestCase(
        "incomplete_escape_sequence_in_triple_quoted_argument",
        # input
        '"""foo\\\nbar"""',
        # output
        Error("error: incomplete escape sequence\n"),
        # extensions
        []
    ),
    TestCase(
        "incomplete_escape_sequence_in_quoted_argument",
        # input
        '"""foo\\',
        # output
        Error("error: incomplete escape sequence\n"),
        # extensions
        []
    ),
    TestCase(
        "triple_quoted_multi_line",
        # input
        '"""The\nquick\r\nbrown\ffox\u0085jumped\u2028over\u2029the\rlazy dog."""',
        # output
        Success("<The\nquick\r\nbrown\ffox\u0085jumped\u2028over\u2029the\rlazy dog.>\n"),
        # extensions
        []
    ),
    TestCase(
        "multiple_triple_quoted_multi_line",
        # input
        '''"""The quick
brown fox""" """jumped
over""" """
the lazy dog.
"""''',
        # output
        Success("""<The quick
brown fox> <jumped
over> <
the lazy dog.
>
"""),
        # extensions
        []
    ),
    TestCase(
        "multiple_triple_quoted_multi_line_with_subdirectives",
        # input
        '''"""The quick
brown fox""" """jumped
over""" { """
the lazy
""" """
dog.""" } ''',
        # output
        Success("""<The quick
brown fox> <jumped
over> [
    <
the lazy
> <
dog.>
]
"""),
        # extensions
        []
    ),
    TestCase(
        "script_latin",
        # input
        "The quick brown fox jumps over the lazy dog",
        # output
        Success("<The> <quick> <brown> <fox> <jumps> <over> <the> <lazy> <dog>\n"),
        # extensions
        []
    ),
    TestCase(
        "script_greek",
        # input
        "Η γρήγορη καφέ αλεπού πηδάει πάνω από το τεμπέλικο σκυλί",
        # output
        Success("<Η> <γρήγορη> <καφέ> <αλεπού> <πηδάει> <πάνω> <από> <το> <τεμπέλικο> <σκυλί>\n"),
        # extensions
        []
    ),
    TestCase(
        "script_cyrillic",
        # input
        "Быстрая коричневая лиса прыгает через ленивую собаку",
        # output
        Success("<Быстрая> <коричневая> <лиса> <прыгает> <через> <ленивую> <собаку>\n"),
        # extensions
        []
    ),
    TestCase(
        "script_hiragana",
        # input
        "素早い茶色のキツネが怠け者の犬を飛び越えます",
        # output
        Success("<素早い茶色のキツネが怠け者の犬を飛び越えます>\n"),
        # extensions
        []
    ),
    TestCase(
        "script_han",
        # input
        "敏捷的棕色狐狸跳过了懒狗",
        # output
        Success("<敏捷的棕色狐狸跳过了懒狗>\n"),
        # extensions
        []
    ),
    TestCase(
        "script_hangul",
        # input
        "빠른 갈색 여우는 게으른 개를 뛰어 넘습니다",
        # output
        Success("<빠른> <갈색> <여우는> <게으른> <개를> <뛰어> <넘습니다>\n"),
        # extensions
        []
    ),
    TestCase(
        "script_thai",
        # input
        "สุนัขจิ้งจอกสีน้ำตาลเร็วกระโดดข้ามสุนัขขี้เกียจ",
        # output
        Success("<สุนัขจิ้งจอกสีน้ำตาลเร็วกระโดดข้ามสุนัขขี้เกียจ>\n"),
        # extensions
        []
    ),
    TestCase(
        "script_hindi",
        # input
        "तेज, भूरी लोमडी आलसी कुत्ते के उपर कूद गई",
        # output
        Success("<तेज,> <भूरी> <लोमडी> <आलसी> <कुत्ते> <के> <उपर> <कूद> <गई>\n"),
        # extensions
        []
    ),
    TestCase(
        "script_emoji",
        # input
        "👨🏻‍🚀",
        # output
        Success("<👨🏻‍🚀>\n"),
        # extensions
        []
    ),
    TestCase(
        "escape_eof",
        # input
        'foo\\',
        # output
        Error("error: illegal escape character\n"),
        # extensions
        []
    ),
    TestCase(
        "line_continuation_before_eof",
        # input
        'foo \\',
        # output
        Error("error: illegal escape character\n"),
        # extensions
        []
    ),
    TestCase(
        "line_continuation_to_eof",
        # input
        'foo \\\n',
        # output
        Success("<foo>\n"),
        # extensions
        []
    ),
    TestCase(
        "lonely_left_brace",
        # input
        "{",
        # output
        Error("error: unexpected '{'\n"),
        # extensions
        []
    ),
    TestCase(
        "lonely_right_brace",
        # input
        "}",
        # output
        Error("error: found '}' without matching '{'\n"),
        # extensions
        []
    ),
    TestCase(
        "empty_braces",
        # input
        "x{}",
        # output
        Success("<x>\n"),
        # extensions
        []
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
"""),
        # extensions
        []
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
"""),
        # extensions
        []
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
"""),
        # extensions
        []
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
"""),
        # extensions
        []
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
"""),
        # extensions
        []
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
"""),
        # extensions
        []
    ),
    TestCase(
        "comment",
        # input
        "# This is a simple comment.",
        # output
        Success(""),
        # extensions
        []
    ),
    TestCase(
        "comment_with_illegal_character",
        # input
        "# This comment contains a forbidden character \x01.",
        # output
        Error("error: illegal character\n"),
        # extensions
        []
    ),
    TestCase(
        "comment_with_a_malformed_character",
        # input
        b"# Malformed UTF-8: \xF0\x28\x8C\xBC", # Overlong encoded SPACE (U+0020).
        # output
        Error("error: malformed UTF-8\n"),
        # extensions
        []
    ),
    TestCase(
        "empty_comment",
        # input
        "#",
        # output
        Success(""),
        # extensions
        []
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
"""),
        # extensions
        []
    ),
    TestCase(
        "error_quoted_unterminated",
        # input
        '"foo \n bar"',
        # output
        Error("error: unclosed quoted\n"),
        # extensions
        []
    ),
    TestCase(
        "error_quoted_illegal",
        # input
        '"foo \a bar"',
        # output
        Error("error: illegal character\n"),
        # extension
        []
    ),
    TestCase(
        "error_quoted_illegal_space",
        # input
        '"foo \\ bar"',
        # output
        Error("error: illegal escape character\n"),
        # extension
        []
    ),
    TestCase(
        "error_missing_closing_curly_brace",
        # input
        """foo {
    bar

""", # missing closing curly brace
        # output
        Error("error: expected '}'\n"),
        # extension
        []
    ),
    TestCase(
        "error_unexpected_closing_curly_brace",
        # input
        """foo 
    bar
}
""", # unpaired curly brace
        # output
        Error("error: found '}' without matching '{'\n"),
        # extensions
        []
    ),
    TestCase(
        "error_unexpected_closing_curly_brace_in_longer_script",
        # input
        """# This is a code comment!
foo bar {
    baz {
        123 456
    } }
}
abc xyz {
    qux
}
""",
        # output
        Error("error: found '}' without matching '{'\n"),
        # extensions
        []
    ),
    TestCase(
        "control_z",
        # input
        "foo\u001A",
        # output
        Success("<foo>\n"),
        # extensions
        []
    ),
    TestCase(
        "control_z_unexpected",
        # input
        "fo\u001Ao",
        # output
        Error("error: illegal character U+001A\n"),
        # extensions
        []
    ),
    TestCase(
        "control_character",
        # input
        "fo\x01o",
        # output
        Error("error: illegal character U+0001\n"),
        # extensions
        []
    ),
    TestCase(
        "unassigned_character",
        # input
        "fo\U000EFFFFo",
        # output
        Error("error: illegal character U+EFFFF\n"),
        # extensions
        []
    ),
    TestCase(
        "lonely_high_surrogate_character",
        # input
        b"fo\xD8\x3Do",
        # output
        Error("error: malformed UTF-8\n"),
        # extensions
        []
    ),
    TestCase(
        "lonely_low_surrogate_character",
        # input
        b"fo\xDE\x00o",
        # output
        Error("error: malformed UTF-8\n"),
        # extensions
        []
    ),
    TestCase(
        "truncated_character",
        # input
        b"\xF0\x9F\x98", # Truncated Grinning Face (U+1F600)
        # output
        Error("error: malformed UTF-8\n"),
        # extensions
        []
    ),
    TestCase(
        "truncated_overlong_character_sequence",
        # input
        b"\xC1", # Truncated overlong encoded sequence.
        # output
        Error("error: malformed UTF-8\n"),
        # extensions
        []
    ),
    TestCase(
        "overlong_character_sequence",
        # input
        b"\xC0\xA0", # Overlong encoded SPACE (U+0020).
        # output
        Error("error: malformed UTF-8\n"),
        # extensions
        []
    ),
    TestCase(
        "invalid_octet_sequence",
        # input
        b"\xF0\x28\x8C\xBC", # Overlong encoded SPACE (U+0020).
        # output
        Error("error: malformed UTF-8\n"),
        # extensions
        []
    ),
    TestCase(
        "invalid_octet_sequence_in_directive",
        # input
        b"foo\xF0\x28\x8C\xBCbar", # Overlong encoded SPACE (U+0020).
        # output
        Error("error: malformed UTF-8\n"),
        # extensions
        []
    ),
    TestCase(
        "private_use_character",
        # input
        "fo\U0010FFFDo",
        # output
        Success("<fo\U0010FFFDo>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_cased_letter_in_argument",
        # input
        "\u0041\u0061\u01C5", # Lu Ll Lt
        # output
        Success("<\u0041\u0061\u01C5>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_letter_in_argument",
        # input
        "\u02B0\u00AA", # Lm Lo
        # output
        Success("<\u02B0\u00AA>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_mark_in_argument",
        # input
        "\u0300\u0903\u0488", # Mn Mc Me
        # output
        Success("<\u0300\u0903\u0488>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_number_in_argument",
        # input
        "\u0030\u16EE\u00B2", # Nd Nl No
        # output
        Success("<\u0030\u16EE\u00B2>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_punctuation_in_argument",
        # input
        "\u005F\u002D\u0028\u0029\u00AB\u00BB\u0021", # Pc Pd Ps Pe Pi Pf Po
        # output
        Success("<\u005F\u002D\u0028\u0029\u00AB\u00BB\u0021>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_symbol_in_argument",
        # input
        "\u002B\u0024\u005E\u00A6", # Sm Sc Sk So
        # output
        Success("<\u002B\u0024\u005E\u00A6>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_other_in_argument",
        # input
        "\u00AD\uE000", # Cf Co
        # output
        Success("<\u00AD\uE000>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_cased_letter_in_quoted_argument",
        # input
        '"\u0041\u0061\u01C5"', # Lu Ll Lt
        # output
        Success("<\u0041\u0061\u01C5>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_letter_in_quoted_argument",
        # input
        '"\u02B0\u00AA"', # Lm Lo
        # output
        Success("<\u02B0\u00AA>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_mark_in_quoted_argument",
        # input
        '"\u0300\u0903\u0488"', # Mn Mc Me
        # output
        Success("<\u0300\u0903\u0488>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_number_in_quoted_argument",
        # input
        '"\u0030\u16EE\u00B2"', # Nd Nl No
        # output
        Success("<\u0030\u16EE\u00B2>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_punctuation_in_quoted_argument",
        # input
        '"\u005F\u002D\u0028\u0029\u00AB\u00BB\u0021"', # Pc Pd Ps Pe Pi Pf Po
        # output
        Success("<\u005F\u002D\u0028\u0029\u00AB\u00BB\u0021>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_symbol_in_quoted_argument",
        # input
        '"\u002B\u0024\u005E\u00A6"', # Sm Sc Sk So
        # output
        Success("<\u002B\u0024\u005E\u00A6>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_other_in_quoted_argument",
        # input
        '"\u00AD\uE000"', # Cf Co
        # output
        Success("<\u00AD\uE000>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_cased_letter_in_triple_quoted_argument",
        # input
        '"""\u0041\u0061\u01C5"""', # Lu Ll Lt
        # output
        Success("<\u0041\u0061\u01C5>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_letter_in_triple_quoted_argument",
        # input
        '"""\u02B0\u00AA"""', # Lm Lo
        # output
        Success("<\u02B0\u00AA>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_mark_in_triple_quoted_argument",
        # input
        '"""\u0300\u0903\u0488"""', # Mn Mc Me
        # output
        Success("<\u0300\u0903\u0488>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_number_in_triple_quoted_argument",
        # input
        '"""\u0030\u16EE\u00B2"""', # Nd Nl No
        # output
        Success("<\u0030\u16EE\u00B2>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_punctuation_in_triple_quoted_argument",
        # input
        '"""\u005F\u002D\u0028\u0029\u00AB\u00BB\u0021"""', # Pc Pd Ps Pe Pi Pf Po
        # output
        Success("<\u005F\u002D\u0028\u0029\u00AB\u00BB\u0021>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_symbol_in_triple_quoted_argument",
        # input
        '"""\u002B\u0024\u005E\u00A6"""', # Sm Sc Sk So
        # output
        Success("<\u002B\u0024\u005E\u00A6>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_other_in_triple_quoted_argument",
        # input
        '"""\u00AD\uE000"""', # Cf Co
        # output
        Success("<\u00AD\uE000>\n"),
        # extensions
        []
    ),
    TestCase(
        "general_category_cased_letter_in_comment",
        # input
        "#\u0041\u0061\u01C5", # Lu Ll Lt
        # output
        Success(""),
        # extensions
        []
    ),
    TestCase(
        "general_category_letter_in_comment",
        # input
        "#\u02B0\u00AA", # Lm Lo
        # output
        Success(""),
        # extensions
        []
    ),
    TestCase(
        "general_category_mark_in_comment",
        # input
        "#\u0300\u0903\u0488", # Mn Mc Me
        # output
        Success(""),
        # extensions
        []
    ),
    TestCase(
        "general_category_number_in_comment",
        # input
        "#\u0030\u16EE\u00B2", # Nd Nl No
        # output
        Success(""),
        # extensions
        []
    ),
    TestCase(
        "general_category_punctuation_in_comment",
        # input
        "#\u005F\u002D\u0028\u0029\u00AB\u00BB\u0021", # Pc Pd Ps Pe Pi Pf Po
        # output
        Success(""),
        # extensions
        []
    ),
    TestCase(
        "general_category_symbol_in_comment",
        # input
        "#\u002B\u0024\u005E\u00A6", # Sm Sc Sk So
        # output
        Success(""),
        # extensions
        []
    ),
    TestCase(
        "general_category_other_in_comment",
        # input
        "#\u00AD\uE000", # Cf Co
        # output
        Success(""),
        # extensions
        []
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
"""),
        # extensions
        []
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
"""),
        # extensions
        []
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
        # extensions
        [],
    ),
    TestCase(
        "document_markup",
        # input
        """chapter "The Raven"
author "Edgar Allan Poe"
section "First Act" {
  paragraph {
    "Once upon a midnight dreary, while I pondered, weak and weary,"
    "Over many a quaint and " bold{"curious"} " volume of forgotten lore-"
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
"""),
        # extensions
        []
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
"""),
        # extensions
        []
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
'''),
        # extensions
        []
    ),
    TestCase(
        "build_script",
        # input
        """project Linux
version 6.14
target kernel {
    flags -Wall
    sources {
        init.c fork.c scheduler.c
        interrupt.c
        deadlock.c panic.c
    }
}""",
        # output
        Success("""<project> <Linux>
<version> <6.14>
<target> <kernel> [
    <flags> <-Wall>
    <sources> [
        <init.c> <fork.c> <scheduler.c>
        <interrupt.c>
        <deadlock.c> <panic.c>
    ]
]
"""),
        # extensions
        []
    ),
    TestCase(
        "ai_training",
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
"""),
        # extensions
        []
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
        # extensions
        []
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

func sum x y {
    add       # Pop the function arguments and push their sum.
    return 1  # One return value is left on the stack.
}
""",
        # output
        Success("""<push> <1>
<push> <2>
<add>
<pop> <$x>
<print> <1 + 2 =>
<print> <$x>
<func> <sum> <x> <y> [
    <add>
    <return> <1>
]
"""),
        # extensions
        []
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
    } else {
        sleep 1s # Lets try again after a moment.
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
    <else> [
        <sleep> <1s>
    ]
]
<exit> <1>
"""),
        # extensions
        []
    ),
    TestCase(
        "shell_commands",
        # input
        """cat myfile.txt

do {
  ./webserver -p 8080
  ./database --api-key 123 --data-dir /var/lib/db/
} > output.txt
""",
        # output
        Success("""<cat> <myfile.txt>
<do> [
    <./webserver> <-p> <8080>
    <./database> <--api-key> <123> <--data-dir> </var/lib/db/>
]
<>> <output.txt>
"""),
        # extensions
        []
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
        # output
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
"""),
        # extensions
        []
    ),
    TestCase(
        "c_single_line_comment",
        # input
        "// This is a single line C comment.",
        # output
        Success(""),
        # extensions
        [CommentExtension()]
    ),
    TestCase(
        "c_single_line_comment_with_illegal_character",
        # input
        "// This comment contains a forbidden character \x01.",
        # output
        Error("error: illegal character\n"),
        # extensions
        [CommentExtension()]
    ),
    TestCase(
        "c_single_line_comment_with_a_malformed_character",
        # input
        b"// Malformed UTF-8: \xF0\x28\x8C\xBC", # Overlong encoded SPACE (U+0020).
        # output
        Error("error: malformed UTF-8\n"),
        # extensions
        [CommentExtension()]
    ),
    TestCase(
        "c_single_line_empty_comment",
        # input
        "//",
        # output
        Success(""),
        # extensions
        [CommentExtension()]
    ),
    TestCase(
        "c_multi_line_comment",
        # input
        """/* This is a
   multi-line comment. */""",
        # output
        Success(""),
        # extensions
        [CommentExtension()]
    ),
    TestCase(
        "c_multi_line_comment_unterminated",
        # input
        "/* This is a multi-line comment.",
        # output
        Error("error: unterminated multi-line comment\n"),
        # extensions
        [CommentExtension()]
    ),
    TestCase(
        "c_comments_intermixed_with_directives",
        # input
        """foo /* This is a multi-line comment. */ bar
/* This is also a multi-line comment { */ baz //{""",
        # output
        Success("""<foo> <bar>
<baz>
"""),
        # extensions
        [CommentExtension()]
    ),
    TestCase(
        "c_multi_line_comment_missing_astrisk",
        # input
        "/",
        # output
        Success("</>\n"),
        # extensions
        [CommentExtension()]
    ),
    TestCase(
        "c_multi_line_comment_with_illegal_character",
        # input
        "/* This comment contains a forbidden character \x01. */",
        # output
        Error("error: illegal character\n"),
        # extensions
        [CommentExtension()]
    ),
    TestCase(
        "c_multi_line_comment_with_a_malformed_character",
        # input
        b"/* Malformed UTF-8: \xF0\x28\x8C\xBC */", # Overlong encoded SPACE (U+0020).
        # output
        Error("error: malformed UTF-8\n"),
        # extensions
        [CommentExtension()]
    ),
    TestCase(
        "c_multi_line_empty_comment",
        # input
        "/**/",
        # output
        Success(""),
        # extensions
        [CommentExtension()]
    ),
    TestCase(
        "expression_argument_empty",
        # input
        "()",
        # output
        Success("<>\n"),
        # extensions
        [ExpressionArgumentsExtension()]
    ),
    TestCase(
        "expression_argument_single_character",
        # input
        "(x)",
        # output
        Success("<x>\n"),
        # extensions
        [ExpressionArgumentsExtension()]
    ),
    TestCase(
        "expression_argument_multi_character",
        # input
        "(x ^ y)",
        # output
        Success("<x ^ y>\n"),
        # extensions
        [ExpressionArgumentsExtension()]
    ),
    TestCase(
        "expression_argument_nested_parentheses",
        # input
        "(x && (y + (1 * z) / 5) - 2)",
        # output
        Success("<x && (y + (1 * z) / 5) - 2>\n"),
        # extensions
        [ExpressionArgumentsExtension()]
    ),
    TestCase(
        "expression_argument_in_directive",
        # input
        """set x = (1 + 2 + 3)
set y = (x * x)
""",
        # output
        Success("""<set> <x> <=> <1 + 2 + 3>
<set> <y> <=> <x * x>
"""),
        # extensions
        [ExpressionArgumentsExtension()]
    ),
    TestCase(
        "expression_argument_with_subdirective",
        # input
        """if ((x + y) == z) {
    print "x + y = " (z)
}
""",
        # output
        Success("""<if> <(x + y) == z> [
    <print> <x + y = > <z>
]
"""),
        # extensions
        [ExpressionArgumentsExtension()]
    ),
    TestCase(
        "expression_arguments_with_no_separating_space",
        # input
        "a(b)(c)x(y)z",
        # output
        Success("<a> <b> <c> <x> <y> <z>\n"),
        # extensions
        [ExpressionArgumentsExtension()]
    ),
    TestCase(
        "expression_argument_unbalanced_parentheses",
        # input
        "(()",
        # output
        Error("error: incomplete expression\n"),
        # extensions
        [ExpressionArgumentsExtension()]
    ),
    TestCase(
        "expression_argument_parentheses_argument",
        # input
        "())",
        # output
        Success("<> <)>\n"),
        # extensions
        [ExpressionArgumentsExtension()]
    ),
    TestCase(
        "expression_argument_with_a_malformed_character",
        # input
        b"(\xF0\x28\x8C\xBC)", # Overlong encoded SPACE (U+0020).
        # output
        Error("error: malformed UTF-8\n"),
        # extensions
        [ExpressionArgumentsExtension()]
    ),
    TestCase(
        "expression_argument_illegal_character",
        # input
        b"(\x01)",
        # output
        Error("error: illegal character\n"),
        # extensions
        [ExpressionArgumentsExtension()]
    ),
    TestCase(
        "punctuator_arguments",
        # input
        "x=y",
        # output
        Success("<x> <=> <y>\n"),
        # extensions
        [PunctuatorArgumentsExtension(["="])]
    ),
    TestCase(
        "punctuator_arguments_by_length_ascending",
        # input
        "x===y",
        # output
        Success("<x> <===> <y>\n"),
        # extensions
        [PunctuatorArgumentsExtension(["=", "==", "==="])]
    ),
    TestCase(
        "punctuator_arguments_by_length_descending",
        # input
        "x===y",
        # output
        Success("<x> <===> <y>\n"),
        # extensions
        [PunctuatorArgumentsExtension(["===", "==", "="])]
    ),
    TestCase(
        "punctuator_arguments_mixed",
        # input
        "x===y",
        # output
        Success("<x> <===> <y>\n"),
        # extensions
        [PunctuatorArgumentsExtension(['==', '=', '==='])]
    ),
    TestCase(
        "punctuator_arguments_multiple_duplicate_starters",
        # input
        "++=-=-",
        # output
        Success("<+> <+=> <-=> <->\n"),
        # extensions
        [PunctuatorArgumentsExtension(["+", "+=", '-=', "-"])]
    ),
    TestCase(
        "punctuator_arguments_multi_byte",
        # input
        "$\u01C4\uFA6D\U0001F600",
        # output
        Success("<\u0024> <\u01C4> <\uFA6D> <\U0001F600>\n"),
        # extensions
        [PunctuatorArgumentsExtension(["\u0024", "\u01C4", "\uFA6D", "\U0001F600"])]
    ),
    TestCase(
        "domain_specific_language_with_all_extensions",
        # input
        """// This is a simple, C-like program.
msg:="Hello, World!"
if(isEmpty(msg)){
    msg="(nil message)"
}
echo msg
""",
        # output
        Success("""<msg> <:=> <Hello, World!>
<if> <isEmpty(msg)> [
    <msg> <=> <(nil message)>
]
<echo> <msg>
"""),
        # extensions
        [CommentExtension(), ExpressionArgumentsExtension(), PunctuatorArgumentsExtension(["=", ":="])]
    ),
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
    Path(f"conformance").mkdir(exist_ok=True)
    with open(f"conformance/{tcase.name}.conf", "wb") as out:
        out.write(bytes)
    if isinstance(tcase.output, Success):
        with open(f"conformance/{tcase.name}.pass", "wb") as out:
            out.write(tcase.output.value.encode("utf-8"))
    else:
        with open(f"conformance/{tcase.name}.fail", "wb") as out:
            out.write(tcase.output.value.encode("utf-8"))
    # Include extensions.
    for extension in tcase.extensions:
        if isinstance(extension, CommentExtension):
            with open(f"conformance/{tcase.name}.ext_c_style_comments", "w", encoding="utf-8") as out:
                pass
        elif isinstance(extension, ExpressionArgumentsExtension):
            with open(f"conformance/{tcase.name}.ext_expression_arguments", "w", encoding="utf-8") as out:
                pass
        elif isinstance(extension, PunctuatorArgumentsExtension):
            with open(f"conformance/{tcase.name}.ext_punctuator_arguments", "w", encoding="utf-8") as out:
                for punct in extension.punctuators:
                    out.write(punct + "\n")

    longest_input = max(longest_input, len(bytes))
    longest_output = max(longest_output, len(tcase.output.value))

# Generate a C header with the test data for the reference implementation.
with open(f"test_suite.h", "w", encoding="utf-8", newline="\n") as out:
    out.write('#include "confetti.h"\n\n')
    out.write("struct TestData {\n")
    out.write("    const char *name;\n")
    out.write(f"    unsigned char input[{longest_input+1}];\n")
    out.write(f"    unsigned char output[{longest_output+1}];\n")
    out.write(f"    conf_extensions extensions;\n")
    out.write("};\n\n")
    for tcase in test_cases:
        for extension in tcase.extensions:
            if isinstance(extension, PunctuatorArgumentsExtension):
                out.write("static const char *" + tcase.name + "[] = {")
                for punctuator in extension.punctuators:
                    out.write(f'"{punctuator}", ')
                out.write("0};\n")
    out.write("\n")
    out.write("static const struct TestData tests_utf8[] = {\n")
    for tcase in test_cases:
        if isinstance(tcase.input, str):
            bytes = tcase.input.encode("utf-8")
        else:
            bytes = tcase.input
        # Test case name.
        out.write(f'    {{ "{tcase.name}", ')
        out.write("{")
        # Input and output.
        for b in bytes:
            out.write("0x{:02X},".format(b))
        out.write("0x00}, {")
        for b in tcase.output.value.encode("utf-8"):
            out.write("0x{:02X},".format(b))
        out.write("0x00}, ")
        # Extensions
        if len(tcase.extensions) > 0:
            out.write("{ ")
            for extension in tcase.extensions:
                if isinstance(extension, CommentExtension):
                    out.write(".c_style_comments=true, ")
                elif isinstance(extension, ExpressionArgumentsExtension):
                    out.write(".expression_arguments=true, ")
                elif isinstance(extension, PunctuatorArgumentsExtension):
                    out.write('.punctuator_arguments=' + tcase.name + ', ')
            out.write("} ")
        else:
            out.write("{ 0 } ")
        out.write("},\n")
    out.write("};\n")
    out.write("\n")
