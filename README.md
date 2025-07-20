# FiL

[![CI Linux Build](https://github.com/FreeYourSoul/FiL/actions/workflows/ci-linux.yml/badge.svg)](https://github.com/FreeYourSoul/FiL/actions/workflows/ci-linux.yml)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/f0e4ad29546841038cd558a38d619e21)](https://app.codacy.com/gh/FreeYourSoul/FiL?utm_source=github.com&utm_medium=referral&utm_content=FreeYourSoul/FiL&utm_campaign=Badge_Grade)
[![Codacy Badge](https://app.codacy.com/project/badge/Coverage/a35205510e714acb9fb438c6f8d1da4a)](https://app.codacy.com/gh/FreeYourSoul/FiL/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_coverage)
![](https://tokei.rs/b1/github/FreeYourSoul/FiL?category=lines)
![License](https://img.shields.io/badge/license-MIT-blue.svg)

Free instrument Library is a general purpose C++ library principally header-only

## CLI

**Header Only**: Command line interface header only library. It is callback based, every command are part of the
`fil::sub_command` class.

In order to use it, a base class for the cli has to be used (internally this class is a sub-class of sub_command) that
provide a parser method taking the input parameter of the main as parameters.

```c++
  fil::command_line_interface cli([]() { /*callback of the command*/ }, "A Simple Command Line tool");

  // with alias -o
  // Add an option ex: ./binary --opt-with-arg "This is an argument"
  cli.add_option(fil::option("--opt-with-arg", "-o", [](std::string arg) { /*handler for the options*/ }, "command with arg"));

  // Add an option ex: ./binary --opt-without-arg
  cli.add_option(fil::option("--opt-without-arg", []() { /*handler for the options without argument required in the opt*/ }, "command with arg"));

  // retrieve the parameters of the command
  std::vector<std::string> cli_argument;
  cli.on_parameter_handler([&cli_argument](std::string param) { cli_argument.emplace_back(std::move(param)); });

  // Parse the command line
  cli.parse_command_line(argc, argv);
```

The command line is made in a modern fashion where it is possible to call options of a cli layer and then call sub-command.

_Notes:_

- It is possible to compose commands with sub_command thanks to the method `add_sub_command`
- Utility function to automatically add a recurrent type of options exists:

```c++
/**
 * Add a an option storing the argument into an output parameter
 *
 * @param sub_command add the option in this sub_command
 * @param opt option code of the option
 * @param argument output parameter in which storing the argument of the option (can be integral or string)
 * @param help helping string displayed when using --help
 */
template<typename T>
add_argument_option(sub_command& sub_command, std::string opt, T& argument, std::string help = "");

/**
 * Do an aggregation of the argument of a sub_command into a vector
 *
 * @param sub_command command to do the aggregation from
 * @param args_string output parameter : aggregate all the parameter into this vector
 */
add_multi_arg(sub_command& sub_command, std::vector<std::string>& args_string);
```

- The --help command is added by default.

## Algorithm & Datastructures

**Header Only** : Some helper algorithm (string algorithm, container and so on) : All algorithm should be properly
documented with doxygen documentation.

## FSM (Finite State Machine)

**Header Only** : Implementation of a simple Finite State Machine.

**This implementation just requires:**

- to define the transition possible from one state to another with a handler defining when a state can pass from one to
  the other.
- to define on_entry state for the state you require specific actions to be done when entering the state
- to use the `advance` method in order to trigger the state machine (go to any other state if possible).

## SOA

`fil::soa` is a datastructure inspired by the ECS (Entity Component System) architecture. It is a data structure that
store data in a way that is cache friendly and allows to iterate over the data in a very efficient way.
It is compatible with c++ standard iterators and algorithms.

The implementation is strongly inspired by [columnist](https://github.com/rollbear/columnist)
from [Björn Fahller](https://github.com/rollbear). A presentastion of the datastructure has been done by Björn in
multiple conferences, the most recent one is [here](https://www.youtube.com/watch?v=QStPbnKgIMU).

> see tests for usage example.

## Database KV

Key value datastore abstraction, this is not header only and require the library to be compiled with specific options to
retrieve the existing policy for the abstraction.
Currently the supported (or soon to be) policy are :

- Rocksdb
- Redis
- Couchbase

Those datastore has been selected as default one as they have the particularity to be usable in-memory. Making them good
choice for personal project.
Cassandra DB, FoundationDB could be interesting choice to have for distributed datastore to be handled by this
abstraction.

## Dependencies

- [fmt](https://github.com/fmtlib/fmt) : Formatting library (C++20 standard)

_If using compiled version of the library with database kv_

- [rocksdb](https://github.com/facebook/rocksdb) : Fast and reliable KV engine developed by facebook, dependency can be
  disabled with the CMake option : `WITH_FIL_ROCKSDB`
- [redis](https://github.com/redis/redis) : Persistent KV in-memory database : `WITH_FIL_REDIS`
- [couchbase](https://github.com/couchbase/libcouchbase) : NoSQL performant database : `WITH_FIL_COUCHBASE`

## Install

### Installation from sources

    ```cmake
    git
    clone
    git@github.com:FreeYourSoul/FiL.git
    cd FiL
    cmake -S . -Bbuild
    sudo cmake --build build --target install
    ```

### Using nix:

Nix is setup on this repository in the .nix folder.
To build and install it. Go to the .nix folder and execute a `nix-build` or `nix-env install`
