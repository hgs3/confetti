#!/usr/bin/env python3

# Confetti: a configuration language and parser library
# Copyright (c) 2025 Henry G. Stratmann III
# Copyright (c) 2025 Confetti Contributors
#
# This file is part of Confetti, distributed under the MIT License.
# For full terms see the included LICENSE file.

# This script builds a two-stage table for referencing Unicode character
# properties in O(1), without branching, and with reasonable compression.
# For details on the algorithm, consult the Unicode Standard 5.1: Data
# Structures for Character Conversion.

from typing import List, Dict, Tuple, Iterable

import os
import sys
import csv
import argparse
from urllib.request import urlretrieve

# Flag(s) assigned to the relevent Unicode character.
IS_FORBIDDEN = 0x1
IS_SPACE = 0x2
IS_PUNCTUATOR = 0x4
IS_ARGUMENT = 0x8
IS_BIDI = 0x10

UNICODE_VERSION = "16.0.0"
MAX_CODEPOINTS = 0x110000  # Defined by the Unicode Consortium.
BUCKET_SIZE = 128  # The number of codepoints in a single stage1 bucket.

Codepoint = int
CharacterFlags = int

# Preallocate a list large enough to accomdiate every Unicode character.
codepoints: List[CharacterFlags] = [0] * MAX_CODEPOINTS

# Python"s CSV library lacks handling "#" comments.
# This function will stip them out.
def decomment(csvfile: Iterable[str]) -> Iterable[str]:
    for row in csvfile:
        raw = row.split("#")[0].strip()
        if raw:
            yield raw

# Helper for parsing ranges of code points as they appear in the Unicode data files.
def parse_range(range: str) -> Tuple[Codepoint, Codepoint]:
    codepoint_range = range.strip().split('..')
    if len(codepoint_range) > 1:
        first_codepoint = int(codepoint_range[0], 16)
        last_codepoint = int(codepoint_range[1], 16)
    else:
        first_codepoint = int(codepoint_range[0], 16)
        last_codepoint = first_codepoint
    return (first_codepoint, last_codepoint + 1)

def compile_table() -> None:
    if not os.path.exists("UnicodeData.txt") or not os.path.exists("PropList.txt"):
        print("error: Please download the Unicode character database first;")
        print(f"       run this script with the '--download' option.")
        sys.exit(1)

    with open(os.path.join("UnicodeData.txt"), encoding="utf-8-sig") as file:
        reader = csv.reader(decomment(file), delimiter=";")
        for record in reader:
            codepoint = int(record[0].strip(), 16)
            name = record[1].strip()
            gc = record[2].strip()
            # Calculate the range of code points for all the CJK Ideograph and Hangul Syllable ranges.
            if ("Ideograph" in name or name.startswith("<Hangul")) and name.endswith("First>"):
                next_line = next(reader)
                next_codepoint = int(next_line[0], 16)
            else:
                next_codepoint = codepoint
            for cp in range(codepoint, next_codepoint + 1):
                if gc in ["Cc", "Cs", "Cn"]:
                    codepoints[cp] = IS_FORBIDDEN
                else:
                    codepoints[cp] = IS_ARGUMENT

    with open(os.path.join("PropList.txt"), encoding="utf-8-sig") as file:
        for record in csv.reader(decomment(file), delimiter=";"):
            start, stop = parse_range(record[0].strip())
            property = record[1].strip()
            if property == "White_Space": # This includes new line characters.
                for cp in range(start, stop):
                    codepoints[cp] = IS_SPACE

    codepoints[ord('"')] = IS_PUNCTUATOR
    codepoints[ord("'")] = IS_PUNCTUATOR
    codepoints[ord('#')] = IS_PUNCTUATOR
    codepoints[ord(';')] = IS_PUNCTUATOR
    codepoints[ord('{')] = IS_PUNCTUATOR
    codepoints[ord('}')] = IS_PUNCTUATOR

    codepoints[ord('\u200E')] |= IS_BIDI
    codepoints[ord('\u200F')] |= IS_BIDI
    codepoints[ord('\u202A')] |= IS_BIDI
    codepoints[ord('\u202B')] |= IS_BIDI
    codepoints[ord('\u202D')] |= IS_BIDI
    codepoints[ord('\u202E')] |= IS_BIDI
    codepoints[ord('\u2066')] |= IS_BIDI
    codepoints[ord('\u2067')] |= IS_BIDI
    codepoints[ord('\u2068')] |= IS_BIDI
    codepoints[ord('\u202C')] |= IS_BIDI
    codepoints[ord('\u2069')] |= IS_BIDI

    # This dictionary is used to keep an ordered set of all unique code points.
    # Many of the code points within the Unicode code space have overlapping properties so
    # only unique code points need to be serialized.
    unique_codepoints: Dict[CharacterFlags, int] = {}

    # Add the 'null' (e.g. zero property values) code point to the set of unique code points.
    # This code point will be used when attempting to retrieve an invalid code point from the C API.
    unique_codepoints[0] = 0

    # Now add all the code points to create an ordered set.
    for cp, flags in enumerate(codepoints):
        if flags not in unique_codepoints:
            unique_codepoints[flags] = len(unique_codepoints)

    # Build a Two-stage table for storing all code points.
    # This is recommended by Chapter 5.1 of The Unicode Standard.
    stage1: List[int] = []
    stage2: List[int] = []
    stage2_tables: List[List[int]] = []

    for code in range(MAX_CODEPOINTS):
        # Only build stage2 tables on bucket boundaries.
        if (code % BUCKET_SIZE) != 0:
            continue

        # Build a stage2 table for the current range of codepoints.
        # This table may be discarded if it's a duplicate of another table.
        table: List[int] = []

        for code2 in range(code, code + BUCKET_SIZE):
            flags = codepoints[code2] # Grab the codepoint.
            if flags in unique_codepoints: # Find its index within the ordered set of code points.
                table += [flags]
                continue
            # Only a subset of the avaiable Unicode character space is mapped to real characters.
            # The current codepoint happens to not exists so default to the null codepoint.
            table += [0]

        # Check if this table was already generated.
        if table in stage2_tables:
            stage1 += [stage2_tables.index(table) * BUCKET_SIZE]
        else:
            stage1 += [len(stage2)]
            stage2 += table
            stage2_tables += [table]

    size = 0
    source = 'uint8_t conf_uniflags(uint32_t cp)\n'
    source += '{\n'

    # Write stage1 table.
    source += '    static const uint16_t stage1_table[] = {'
    for index, value in enumerate(stage1):
        if (index % 8) == 0:
            source += '\n'
            source += '        '
        source += "{0}, ".format(value)
        size += 2
    source += '\n'
    source += '    };\n\n'

    # Write stage2 table.
    source += '    static const uint8_t stage2_table[] = {'
    for index, value in enumerate(stage2):
        if (index % 8) == 0:
            source += '\n'
            source += '        '
        source += "{0}, ".format(value)
        size += 1
    source += '\n'
    source += '    };\n\n'

    source += '    // LCOV_EXCL_START\n'
    source += '    assert(cp < {0});\n'.format(MAX_CODEPOINTS)
    source += '    // LCOV_EXCL_STOP\n'
    source += '    const int stage2_offset = stage1_table[cp >> {0}];\n'.format(BUCKET_SIZE.bit_length() - 1)
    source += '    return stage2_table[stage2_offset + (cp & {0})];\n'.format(BUCKET_SIZE - 1)
    source += '}\n\n'

    file = open("confetti_unidata.c", "w", encoding="utf-8")
    file.write("// Do NOT edit this file. It was programtically generated with {0}.\n".format(os.path.basename(__file__)))
    file.write("// it contains {0} kB worth of Unicode character data.\n".format(size / 1024))
    file.write("\n")
    file.write('#include <stdint.h>\n')
    file.write("#include <assert.h>\n")
    file.write("\n")
    file.write(source)
    file.close()

def download_unicode_database() -> None:
    urls = [
        f"https://www.unicode.org/Public/{UNICODE_VERSION}/ucd/UnicodeData.txt",
        f"https://www.unicode.org/Public/{UNICODE_VERSION}/ucd/PropList.txt",
    ]
    for url in urls:
        download_file = os.path.basename(url)
        if not download_file.endswith(".txt"):
            download_file += ".txt"
        print("downloading", url)
        urlretrieve(url, download_file)

if __name__ == "__main__":
    class Args(argparse.Namespace):
        def __init__(self) -> None:
            super().__init__()
            self.download = False

    parser = argparse.ArgumentParser(description="Generate Unicode data.")
    parser.add_argument("--download", dest="download", action="store_true", help="download the Unicode Character Database from Unicode.org")

    args = Args()
    parser.parse_args(namespace=args)

    if args.download:
        download_unicode_database()
    else:
        compile_table()
