# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Recent Changes]

---

## 1.2.0

- Implementation of `fil/copa` : a combinatorial recusive descent parser to implement Domain Specific Language
  interpretor or compiler (DSL).

---

## 1.1.0

- Implementation of custom data structures in `fil/datastructure` :
    - soa : structure of array data structure: strongly inspired by Bjorn Faller implementation of std::hive

---

## 1.0.0

Initial Project Setup

- Implementation of CLI in `fil/cli` : a CLI header-only library
- Implementation of file in `fil/file` : a File reader to efficiently read data from disk while working on buffer of big
  data read.
- Implementation of finite state machine in `fil/fsm` ; a simple state machine that can be used
- Implementation of custom algorithm in `fil/algorithm`
    - `contains.hh` : `all_contains` : check that all elements from a vector is inside another one (order independant)
    - `string.hh`:
        - `split_string` : split a string into chunks
        - `join` : join a vector of string into a single one
        - `rtrim` : trim a string to the right
        - `ltrim` : trim a string to the left

### Project: FiL (Free Instrumental Library)

A modern C++26 header-only library providing diverse utilities.

**Key Technologies**:

- C++26 standard
- Header-only library design
- CMake build system
- Nix flakes for development environment
- GitHub Actions CI/CD
- Code coverage reporting (gcovr)

---

## Notes

### Future Improvements

Based on the current state, areas for enhancement include:

- Extended error messages and diagnostics
- Performance optimizations for deeply nested rules
- Additional built-in matchers for common patterns
- Better integration with standard library types

---

**Repository**: [FreeYourSoul/FiL](https://github.com/FreeYourSoul/FiL)

Last Updated: 2026-04-06