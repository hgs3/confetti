# Conformance Test Suite

If you create your own implementation of Confetti, then you are encouraged to test it against the official conformance test suite, whose test cases are located in the [tests/conformance](conformance/) directory of this repository.

Each test case is presented as a file on the file system:

Files ending with the extension `.conf` are Confetti files.
These are the input files you feed to your Confetti parser.

Files ending with the extensions `.pass` and `.fail` indicate whether the Confetti is valid or invalid.
If the input is valid Confetti, there is a `.pass` file.
If the input is invalid Confetti, there is a `.fail` file.

The contents of the `.pass` file is each directive pretty printed on its own line with square brackets used to denote subdirectives.
Each directive argument is enclosed in angle brackets.
When you parse a valid Confetti file, you can generate output that matches this format for direct comparison.

The contents of the `.fail` file is a readable string describing why the Confetti input is invalid.
On error, your parser can emit an identical message or you can emit your own custom error message.
The error messages in the `.fail` file are used by the C implementation of Confetti and _can_ change between releases.

## Example

Assume there is a `.conf` file named `sample.conf` with the contents:

```
 foo  bar
{
      baz
 }
```

The contents of this file is valid Confetti, so there will be a separate file named `sample.pass` with the contents:

```
<foo> <bar> [
    baz
]
```

Notice how the contents of the `.pass` file is, effectively, the input file contents but pretty printed with arguments and subdirectives clearly noted.

## Extensions

Additionally, each test case _may_ have files ending with the extensions listed in the **following table**.
The presence of these files indicates if the test is to be interpreted as needing a specific Confetti language extension, as listed in the annex of the Confetti specification.

You can ignore tests that require an extension your Confetti implementation does not support.

Note that a single test case can depend on zero, one, or multiple extensions.

| Extension | Description
| --- | --- |
| `.ext_c_style_comments` | Indicates the comment syntax extension is needed. The file contents can be ignored. |
| `.ext_expression_arguments` | Indicates the expression arguments extension is needed. The file contents can be ignored. |
| `.ext_punctuator_arguments` | Indicates the punctuator arguments extension is needed. This file contains a line-separated list of punctuator arguments. |

# How Confetti is Tested

The canonical C implementation of Confetti is tested in the following ways:

* Official conformance test suite
* 100% branch coverage
* Manually written tests
* Out-of-memory tests
* Fuzz tests
* Static analysis
* Valgrind analysis
* Code sanitizers (UBSAN, ASAN, and MSAN)
* Extensive use of assert() and run-time checks

You can run the test suite by executing the `test.sh` shell script from this directory.
Note that the test suite _does_ depend on [Audition](https://railgunlabs.com/audition/).
