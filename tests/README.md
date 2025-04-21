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
