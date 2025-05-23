﻿# Confetti

**Confetti** is a configuration language and parser library written in C11 with no external dependencies.

Confetti is intended for human-editable configuration files.
It is minimalistic, untyped, and unopinionated.
Schema conformance is validated by the user application.

Confetti does **not** compete with JSON or XML, it competes with [INI files](https://en.wikipedia.org/wiki/INI_file).

[![Build Status](https://github.com/hgs3/confetti/actions/workflows/build.yml/badge.svg)](https://github.com/hgs3/confetti/actions/workflows/build.yml)&nbsp;
[![Documentation](https://img.shields.io/badge/Man_Pages-online-blue)](https://confetti.hgs3.me/manual/)

## Example

```conf
# This is a comment.

probe-device eth0 eth1

user * {
    login anonymous
    password "${ENV:ANONPASS}"
    machine 167.89.14.1
    proxy {
        try-ports 582 583 584
    }
}

user "Joe Williams" {
    login joe
    machine 167.89.14.1
}
```

Browse many [more examples here](https://confetti.hgs3.me/examples/).

## Language Features

* [Can be learned in minutes](https://confetti.hgs3.me/learn/)
* [Language grammar fits on a postcard](https://confetti.hgs3.me/specification/#_lexical_grammar)
* Typeless design
* Unicode® conformant
* Localization friendly

## Installation

Download the [latest release tarball](https://github.com/hgs3/confetti/releases/) and build with

```
$ ./configure
$ make
$ make install
```

or build with CMake

```
$ cmake -B build
$ cmake --build build --config Release
$ cmake --install build --config Release
```

Code examples are available in the [examples](examples/) directory.

## Local Development

Install **Python 3.12** or newer and the [Audition testing framework](https://railgunlabs.com/audition/).
Clone the repository with Git and use CMake for local development and testing.
Review [tests/README.md](tests/README.md) for more details.

## Documentation

Confetti language documentation is available on [its website](https://confetti.hgs3.me/).

The C API is documented by its man pages.
Run `man confetti` after installing the library or [view them online](https://confetti.hgs3.me/manual/).

## License

MIT License.
See [LICENSE](LICENSE) for details.
