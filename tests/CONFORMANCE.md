# Conformance Testing

If you create your own implementation of Confetti, you can test it against the **official** conformance test suite, located in the [tests/conformance](conformance/) directory of this repository.

Each test case is represented as a file on the file system:

* Files ending with the `.conf` extension are Confetti input files that you feed to your Confetti parser.

* Files ending with the `.pass` and `.fail` extensions indicate whether the Confetti is valid or invalid:
    * A `.pass` file indicates that the input is valid Confetti.
    * A `.fail` file indicates that the input is invalid Confetti.

> [!NOTE]
> It is recommended to glob the `.conf` files rather than hardcode their names, as they can potentially be renamed. Globbing also ensures new tests are automatically detected by your test harness.

## _Pass_ File Format

The contents of the `.pass` file are as follows:

* Each directive is pretty-printed on its own line.

* Square brackets are used to denote subdirectives.

* Each argument of a directive is enclosed in angle brackets (< >).

When you parse a valid Confetti file, your parser should output this format for direct comparison.

## _Fail_ File Format

The contents of the `.fail` file is a readable string describing why the Confetti input is invalid.
When an error occurs, your parser should emit either the same message or a custom error message.

> [!CAUTION]
> The error messages in the `.fail` file are used by the C implementation of Confetti and **may** change between releases.

## Example

Assume there is a `.conf` file named `sample.conf` with the following contents:

```
 foo  bar
{
      baz
 }
```

This file contains valid Confetti, so there will be a separate file named `sample.pass` with the contents:

```
<foo> <bar> [
    baz
]
```

As you can see, the `.pass` file is a pretty-printed version of the input file, where each directive and subdirective is clearly identified.

## Extensions

In addition to the standard `.conf`, `.pass`, and `.fail` files, each test case **may** include additional files with the following extensions.
The presence of these files indicates that the test case requires a specific Confetti language extension, as listed in the annex of the Confetti specification.

You can safely ignore tests that require extensions that your Confetti implementation does not support.

A single test case may depend on zero, one, or multiple extensions.

| Extension | Description
| --- | --- |
| `.ext_c_style_comments` | Indicates the **comment syntax extension** is required. The file contents can be ignored. |
| `.ext_expression_arguments` | Indicates the **expression arguments extension** is required. The file contents can be ignored. |
| `.ext_punctuator_arguments` | Indicates the **punctuator arguments extension** is required. This file contains a line-separated list of punctuator arguments. |
