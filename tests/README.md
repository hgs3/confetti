# How Confetti is Tested

* 100% branch coverage
* Manually written tests
* Out-of-memory tests
* Fuzz tests
* Static analysis
* Valgrind analysis
* Code sanitizers (UBSAN, ASAN, and MSAN)
* Extensive use of assert() and run-time checks

You can run the test suite by executing the `test.sh` shell script from this directory.

## Conformance Test Suite

If you create your own implementation of Confetti, then you should test it against the official conformance test suite.
The test cases are located in the [suite](suite/) directory.

Files ending with the extension `.in` are the input files.
You feed these to your Confetti parser.

Files ending with the extensions `.out` and `.err` are the output files.
If the input is valid Confetti, then there is an `.out` file.
If the input is invalid Confetti, then there is an `.err` file.

The `.out` files print each directive on their own line and uses square brackets to represent subdirectives.
Each directive argument is enclosed in angle brackets.
When you parse a valid Confetti file, you can generate output that matches this format for direct comparison.

The `.err` files contain a readable string describing why the Confetti input is invalid.
