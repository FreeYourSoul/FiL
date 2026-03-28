# FiL

[![CI Linux Build](https://github.com/FreeYourSoul/FiL/actions/workflows/ci-linux.yml/badge.svg)](https://github.com/FreeYourSoul/FiL/actions/workflows/ci-linux.yml)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/f0e4ad29546841038cd558a38d619e21)](https://app.codacy.com/gh/FreeYourSoul/FiL?utm_source=github.com&utm_medium=referral&utm_content=FreeYourSoul/FiL&utm_campaign=Badge_Grade)
[![Codacy Badge](https://app.codacy.com/project/badge/Coverage/a35205510e714acb9fb438c6f8d1da4a)](https://app.codacy.com/gh/FreeYourSoul/FiL/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_coverage)
![](https://tokei.rs/b1/github/FreeYourSoul/FiL?category=lines)
![License](https://img.shields.io/badge/license-MIT-blue.svg)

Free instrument Library is a general purpose C++ library principally header-only.

## Features Summary

FiL provides various modern C++ features:

- **[CLI](doc/cli.md)**: Callback-based command line interface library.
- **[Algorithms & Datastructures](doc/algorithm_datastructures.md)**: Common string algorithms, RNG, and specialized
  containers.
- **[File Reader](doc/file_reader.md)**: High-performance, memory-efficient file reading.
- **[FSM](doc/fsm.md)**: Simple Finite State Machine implementation.
- **[SOA](doc/soa.md)**: Cache-friendly Structure of Arrays data structure.
- **[Database KV](doc/database_kv.md)**: Abstraction for Key-Value datastores (RocksDB, Redis, etc.).
- **[Descpa](doc/descpa.md)**: Declarative Simple Combinator Parser.

## Dependencies

- [fmt](https://github.com/fmtlib/fmt) : Formatting library (C++20 standard)

_If using compiled version of the library with database kv_

- [rocksdb](https://github.com/facebook/rocksdb) : Fast and reliable KV engine developed by Meta : `WITH_FIL_ROCKSDB`
- [redis](https://github.com/redis/redis) : Persistent KV in-memory database : `WITH_FIL_REDIS`
- [couchbase](https://github.com/couchbase/libcouchbase) : NoSQL performant database : `WITH_FIL_COUCHBASE`

## Install

### Installation from sources

```cmake
git clone git@github.com:FreeYourSoul/FiL.git
cd FiL
cmake -S . -Bbuild
sudo cmake --build build --target install
```

### Using nix:

Nix Flakes is supported on this repository.

To build and install it using Nix Flakes:

```bash
nix build
# To install the package in your profile:
nix profile install
```

For development, you can use:

```bash
nix develop
```
