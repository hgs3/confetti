#!/usr/bin/env bash

# Verify lcov is installed.
if ! command -v lcov &> /dev/null; then
    echo "lcov could not be found; please install it to gather code coverage"
    exit 1
fi

# Capture code coverage data.
lcov --capture --directory . --output-file coverage.info --rc branch_coverage=1

# Remove third-party libraries.
lcov --remove coverage.info '/usr/*' -o coverage.info --rc branch_coverage=1

# Get coverage percentage.
COVERAGE=$(lcov --summary coverage.info --rc branch_coverage=1 | grep 'branches' | awk '{print $2}' | sed 's/%//')

# Bash does not support comparing floating-point numbers;
# remove the fractional part from the percentage.
COVERAGE=${COVERAGE%.*}

# Minimum coverage threshold.
THRESHOLD=100

# Check if coverage is above threshold.
if [ "$COVERAGE" -lt "$THRESHOLD" ]; then
    echo "Code coverage is below the threshold: $COVERAGE% < $THRESHOLD%"
    exit 1
fi
