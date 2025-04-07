# Confetti: a configuration language and parser library
# Copyright (c) 2025 Confetti Contributors
#
# This file is part of Confetti, distributed under the MIT License
# For full terms see the included LICENSE file.
# <https://RailgunLabs.com/license>.

from typing import Iterator
from collections.abc import Sized, Iterator

class Confetti:
    def __init__(self, source: str) -> None: ...
    # years since 1900
    def get_root(self) -> Directive: ...

class Directive(Sized):
    def __len__(self) -> int: ...
    def __iter__(self) -> DirectiveIterator: ...
    @property
    def args(self) -> ArgumentIterator: ...

class DirectiveIterator(Iterator[Directive]):
    def __next__(self) -> Directive: ...

class Argument:
    @property
    def value(self) -> str: ...

class ArgumentIterator(Sized, Iterator[Argument]):
    def __len__(self) -> int: ...
    def __iter__(self) -> ArgumentIterator: ...
    def __next__(self) -> Argument: ...
