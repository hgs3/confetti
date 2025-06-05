#!/usr/bin/env bash

# Verify exactly two arguments are provided.
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <exectuable> <file>" >&2
    exit 1
fi

# Run the example exectuable capturing its stderr.
# Example output might look like:
#
#   =================================================================
#   ==377==ERROR: LeakSanitizer: detected memory leaks
#
#   Direct leak of 320 byte(s) in 1 object(s) allocated from:
#      #0 0x7ffff7682c38 in __interceptor_realloc ../../../../src/libsanitizer/asan/asan_malloc_linux.cpp:164
#      #1 0x555555558c37 in readstdin examples/_readstdin.c:57
#
#   SUMMARY: AddressSanitizer: 320 byte(s) leaked in 1 allocation(s).
#
stderr=$($1 < $2 2>&1 1>/dev/null)

# If any stderr was captured, report a filure.
if [ -n "$stderr" ]; then
    echo "$stderr"
    exit 1
fi
