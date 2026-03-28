# CLI (Command Line Interface)

FiL provides a modern, header-only Command Line Interface library. it is callback-based, where every command is part of
the `fil::sub_command` class.

## Basic Usage

To use it, you need to use a base class for the CLI (typically `fil::command_line_interface`, which is a sub-class of
`sub_command`) that provides a `parse_command_line` method.

```cpp
#include <fil/cli/command_line_interface.hh>
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char** argv) {
    // Define the main CLI command with its callback
    fil::command_line_interface cli([]() { 
        std::cout << "Executing main command" << std::endl; 
    }, "A Simple Command Line tool");

    // Add an option with an argument: ./binary --opt-with-arg "argument value"
    // Alias -o is also provided
    cli.add_option(fil::option("--opt-with-arg", "-o", [](std::string arg) {
        std::cout << "Option with arg: " << arg << std::endl;
    }, "command with arg"));

    // Add an option without argument: ./binary --opt-without-arg
    cli.add_option(fil::option("--opt-without-arg", []() {
        std::cout << "Option without arg triggered" << std::endl;
    }, "command without arg"));

    // Handle positional parameters
    std::vector<std::string> cli_arguments;
    cli.on_parameter_handler([&cli_arguments](std::string param) {
        cli_arguments.emplace_back(std::move(param));
    });

    // Parse the command line
    cli.parse_command_line(argc, argv);

    return 0;
}
```

## Advanced Features

### Sub-commands

You can compose commands by adding sub-commands to any `sub_command` (including the main `cli` object).

```cpp
auto& sub = cli.add_sub_command("sub", []() {
    std::cout << "Sub-command executed" << std::endl;
}, "A sub-command description");

sub.add_option(fil::option("--sub-opt", []() {
    std::cout << "Sub-command option executed" << std::endl;
}, "Sub-command option"));
```

### Utility Functions

FiL provides utility functions to simplify common tasks:

#### Storing argument directly

```cpp
int value = 0;
// Automatically adds an option that stores the argument into the 'value' variable
fil::add_argument_option(cli, "--value", value, "An integer value");
```

#### Aggregating multiple arguments

```cpp
std::vector<std::string> all_args;
// Aggregates all remaining positional parameters of a sub-command into a vector
fil::add_multi_arg(cli, all_args);
```

## Help Command

The `--help` command is automatically added by default to all commands and sub-commands, displaying the descriptions
provided during setup.
