# Conformance Test Suite

If you create your own implementation of Confetti, then you should test it against the official conformance test suite.
The test cases are located in the [suite](suite/) directory.

Files ending with the extension `.conf` are the input files.
You feed these to your Confetti parser.

Files ending with the extensions `.pass` and `.fail` indicate whether the Confetti is valid or invalid .
If the input is valid Confetti, then there is an `.pass` file.
If the input is invalid Confetti, then there is an `.fail` file.

The contents of the `.pass` file is each directive pretty printed on its own line and using square brackets to represent subdirectives.
Each directive argument is enclosed in angle brackets.
When you parse a valid Confetti file, you can generate output that matches this format for direct comparison.

The contents of the `.fail` file is a readable string describing why the Confetti input is invalid.

Additionally, a test _may_ have files ending with the extensions listed in the following table.
These files indicate if the test is to be interpreted as needing a specific Confetti language extension, as listed in the annex of the Confetti specification.
You can ignore the tests that require extensions your Confetti implemntation does not support.

| Extension | Description
| --- | --- |
| `.ext_c_style_comments` | Indicates the comment syntax extension is needed. The file contents can be ignored. |
| `.ext_expression_arguments` | Indicates the expression arguments extension is needed. The file contents can be ignored. |
| `.ext_punctuator_arguments` | Indicates the punctuator arguments extension is needed. This file contains a line-separated list of punctuator arguments. |

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
