#!/usr/bin/env bash

# Verify lcov is installed.
if ! command -v lcov &> /dev/null; then
    echo "lcov could not be found; please install it to gather code coverage"
    exit 1
fi

# Capture code coverage data.
lcov --capture --directory . --output-file coverage.info --rc lcov_branch_coverage=1

# Remove third-party libraries.
lcov --remove coverage.info '/usr/*' -o coverage.info --rc lcov_branch_coverage=1

# Generate an HTML code coverage report.
genhtml coverage.info --output-directory coverage --rc lcov_branch_coverage=1
