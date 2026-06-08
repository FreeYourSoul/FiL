# CLI (Command Line Interface)

FiL provides a modern, header-only Command Line Interface library. It is callback-based, where every command is defined
using the `fil::sub_command` class.

## Basic Usage

To use it, you create an instance of `fil::command_line_interface` (which is a subclass of `fil::sub_command`). It
provides the `parse_command_line` method to process `argc` and `argv`.

### Simple Example

```cpp
#include <fil/cli/command_line_interface.hh>
#include <iostream>

int main(int argc, char** argv) {
    // Define the main CLI command with its callback and description
    fil::command_line_interface cli([]() { 
        std::cout << "Executing main command" << std::endl; 
    }, "A Simple Command Line tool");

    // Add an option with an argument: ./binary --name "John"
    cli.add_option(fil::option("--name", "-n", [](std::string arg) {
        std::cout << "Hello, " << arg << "!" << std::endl;
    }, "The name to greet"));

    // Add a flag (option without argument): ./binary --verbose
    cli.add_option(fil::option("--verbose", []() {
        std::cout << "Verbose mode enabled" << std::endl;
    }, "Enable verbose output"));

    // Parse the command line
    cli.parse_command_line(argc, argv);

    return 0;
}
```

## Options and Arguments

Options can be defined with or without an alias, and can take different types of arguments (string, int64_t, or no
argument).

### Option Types

- **Flag**: `fil::option("--flag", []() { ... }, "Help text")`
- **String Argument**: `fil::option("--opt", [](std::string s) { ... }, "Help text")`
- **Integer Argument**: `fil::option("--count", [](std::int64_t i) { ... }, "Help text")`

### Positional Parameters

You can handle positional parameters (arguments not associated with an option) by providing a parameter handler:

```cpp
cli.on_parameter_handler([](std::string param) {
    std::cout << "Positional parameter: " << param << std::endl;
});
```

## Advanced Features

### Sub-commands

You can create complex command hierarchies by adding sub-commands to the main `cli` object or other sub-commands.

```cpp
auto& config = cli.add_sub_command("config", []() {
    std::cout << "Configuring..." << std::endl;
}, "Configuration commands");

config.add_option(fil::option("--set", [](std::string val) {
    // ...
}, "Set a config value"));
```

### Utility Functions

The `fil::cli` namespace provides helpers for common patterns:

#### Storing Arguments Directly

Automatically creates an option that stores its value into a variable:

```cpp
int port = 8080;
fil::cli::add_argument_option(cli, "--port", port, "Port number");
```

#### Aggregating Positional Arguments

Collects all remaining positional arguments into a vector:

```cpp
std::vector<std::string> files;
fil::cli::add_multi_arg(cli, files);
```

## Help Generation

The `--help` (and `-h`) flag is automatically added to all commands and sub-commands. It generates a formatted help
message based on the descriptions provided.
