#!/usr/bin/env bash

# Fuzzer configuration.
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1
export AFL_SKIP_CPUFREQ=1
export AFL_AUTORESUME=1

# Compiler configuration.
export CC=afl-clang
export CXX=afl-clang++

# Project build folder.
OUTDIR=build

# Don't accidentally overwrite the contents of a users working directory.
# Give them a chance to backup their work.
if [ -d "${OUTDIR}" ]; then
    echo "The '${OUTDIR}' directory already exists. Please rename or remove it."
    echo "If you did not create this directory, then it was probably created"
    echo "by the last execution of this script and is safe to manually delete."
    exit 1
fi

# Build the project.
cmake .. -B ${OUTDIR} -G "Ninja" -DCONFETTI_ADDRESS_SANITIZER=ON -DCONFETTI_BUILD_EXAMPLES=ON -DCONFETTI_BUILD_TESTS=ON
cmake --build ${OUTDIR}

# Execute the program.
afl-fuzz -t 250 -i ./corpus/ -o . -S fuzz -- ./${OUTDIR}/examples/parse
