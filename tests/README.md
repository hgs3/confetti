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
This script is run by GitHub Actions and, to run all tests, requires [CMake](https://cmake.org/), the [Ninja](https://ninja-build.org/) build system, [Audition](https://railgunlabs.com/audition/), LCOV for Code Coverage, Valgrind, Clang, and GCC.
You can preview how it is run and how the dependencies are installed, for Linux, by reviewing the [GitHub Actions workflow script](../.github/workflows/build.yml).
