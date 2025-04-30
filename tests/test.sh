#!/usr/bin/env bash
# This script builds Audition with Clang sanitizers and performs Valgrind analysis.

# Destination folder.
OUTDIR=build

# Verify this script is being run from the same directory in which it resides.
if [ ! -e "test_suite.h" ]; then
    echo "Please run this script from the tests/ directory."
    exit 1
fi

# Don't accidentally overwrite the contents of a users working directory.
# Give them a chance to backup their work.
if [ -d "${OUTDIR}" ]; then
    echo "The '${OUTDIR}' directory already exists. Please rename or remove it."
    echo "If you did not create this directory, then it was probably created"
    echo "by the last execution of this script and is safe to manually delete."
    exit 1
fi

# All subsequent commands should kill the script on error.
set -e

# Check code coverage.
if command -v gcc &> /dev/null; then
    CC=gcc cmake .. -B ${OUTDIR} -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCONFETTI_BUILD_TESTS=ON -DCONFETTI_CODE_COVERAGE=ON -DCONFETTI_BUILD_EXAMPLES=OFF
    cmake --build ${OUTDIR}
    cmake --build ${OUTDIR}
    ctest --test-dir ${OUTDIR} --output-on-failure
    cmake --build ${OUTDIR} --target check-coverage
    cmake -E remove_directory ${OUTDIR}
else
    echo "Code coverage requires GCC."
fi

# Perform a standard build with Valgrind analysis.
# Valgrind generates the most detailed output when run against a debug build.
if command -v valgrind &> /dev/null; then
    cmake .. -B ${OUTDIR} -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCONFETTI_BUILD_TESTS=ON -DCONFETTI_BUILD_EXAMPLES=OFF
    cmake --build ${OUTDIR}
    ctest --test-dir ${OUTDIR} --output-on-failure -T memcheck
    cmake -E remove_directory ${OUTDIR}
else
    echo "Valgrind not found; skipping valgrind analysis."
fi

# Clang sanitizers can only be run with Clang (duh).
if command -v clang &> /dev/null; then
    # Run the Undefined Behavior Sanitizer.
    CC=clang cmake .. -B ${OUTDIR} -G "Ninja" -DCONFETTI_BUILD_TESTS=ON -DCONFETTI_UNDEFINED_BEHAVIOR_SANITIZER=ON -DCONFETTI_BUILD_EXAMPLES=OFF
    cmake --build ${OUTDIR}
    ctest --test-dir ${OUTDIR} --output-on-failure
    cmake -E remove_directory ${OUTDIR}

    # Run the Address Sanitizer.
    CC=clang cmake .. -B ${OUTDIR} -G "Ninja" -DCONFETTI_BUILD_TESTS=ON -DCONFETTI_ADDRESS_SANITIZER=ON -DCONFETTI_BUILD_EXAMPLES=OFF
    cmake --build ${OUTDIR}
    ctest --test-dir ${OUTDIR} --output-on-failure
    cmake -E remove_directory ${OUTDIR}

    # Run the Memory Sanitizer.
    CC=clang cmake .. -B ${OUTDIR} -G "Ninja" -DCONFETTI_BUILD_TESTS=ON -DCONFETTI_MEMORY_SANITIZER=ON -DCONFETTI_BUILD_EXAMPLES=OFF
    cmake --build ${OUTDIR}
    ctest --test-dir ${OUTDIR} --output-on-failure
    cmake -E remove_directory ${OUTDIR}
else
    echo "Re-run with CC=Clang for UBSAN, ASAN, MSAN testing."
fi

# Perform a release build to ensure no bugs were hiding in debug mode.
cmake .. -B ${OUTDIR} -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCONFETTI_BUILD_TESTS=ON -DCONFETTI_BUILD_EXAMPLES=OFF
cmake --build ${OUTDIR}
ctest --test-dir ${OUTDIR} --output-on-failure
cmake -E remove_directory ${OUTDIR}
