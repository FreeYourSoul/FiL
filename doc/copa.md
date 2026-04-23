# copa (COmbinator PArser)

`copa` is a C++26 header-only combinator-based parser library. It allows you to build complex recursive descent parsers
by composing simple rules and mapping them directly to C++ structures (AST objects).

## Table of Contents

- [Key Concepts](#key-concepts)
- [Basic Usage](#basic-usage)
    - [Example: A Simple Command Parser](#example-a-simple-command-parser)
- [Available Matchers](#available-matchers)
    - [Basic matchers](#basic-matchers)
    - [Rule Composition](#rule-composition)
    - [Optional matcher](#optional-matcher)
- [Provided Helpers](#provided-helpers)
- [Performance Considerations](#performance-considerations)
    - [Avoiding `tuple_rule` in `or_rule`](#avoiding-tuple_rule-in-or_rule)
- [Mapping to AST](#mapping-to-ast)
- [Integrating with Readers](#integrating-with-readers)
- [Copa Reader](#copa-reader)
    - [Reader Concept](#reader-concept)
    - [Core Requirements](#core-requirements)
    - [The Shallow Copy Concept](#the-shallow-copy-concept)
    - [Implementation Steps](#implementation-steps)

## Key Concepts

- **Rule**: A basic building block that matches a part of the input. Rules can be simple (match a character) or
  complex (composed of other rules).
- **Matcher**: A specific implementation of a rule (e.g., `match_char`, `match_string`).
- **Production**: A high-level grammar definition that combines rules with an AST object and a result convertor.
- **Sink (Aggregator)**: Collects results from matchers and populates the AST object.

## Basic Usage

To use `copa`, you define a **Production**. A production is a struct or class that defines:

1. `ast_object`: The type that will hold the parsing results.
2. `rules()`: A static method returning the composed rules.
3. `convertor()`: A static method returning an aggregator for the `ast_object`.

### Example: A Simple Command Parser

The following example parses a string of format: "CMD {command_name} ON {target} {options} "

```cpp
#include <iostream>
#include <vector>

#include <fil/copa/copa.hh>
#include <fil/copa/matcher.hh>
#include <fil/copa/sink.hh>
#include <fil/meta/buffer_reader.hh>

// 1. Define your AST structure
struct MyCommand {
    std::string command;
    std::string target;
    std::vector<std::string> options;
};

// 2. Define the Grammar Production
struct CommandGrammar {
    using ast_object = MyCommand;

    static constexpr auto rules() {
        using namespace fil::copa;
        
        // Rule: "CMD" <id:command> "ON" <id:target> [options...]
        return match_string<fixed_string{"CMD"}>{}
             + match_identifier<member<&MyCommand::command>>{}
             + match_string<fixed_string{"ON"}>{}
             + match_identifier<member<&MyCommand::target>>{}
             + list_rule<match_identifier<member<&MyCommand::options>>>{};
    }

    static constexpr auto convertor() {
        return fil::copa::sink::aggregator<ast_object>{};
    }
};

int main() {
    std::string input = "CMD start ON engine turbo fast";
    fil::buffer_reader reader(std::move(input));
    CommandGrammar grammar;

    // 3. Parse the input
    auto result = fil::copa::parse(grammar, std::move(reader));

    // result.has_value() == true
    // result->command    == start
    // result->target     == engine
    // result->options    == { turbo, fast }
}
```

## Available Matchers

`copa` provides several built-in matchers in the `fil::copa` namespace:

> **Greedy Matcher**: Copa design is based on greedy matching. If a matcher can match multiple times, it will match
> until it can't anymore. For instance, this means that it is impossible to match a list of integer followed by an
> integer of a specific value. As the list would consume that integer.  
> The design of the parser must take that into account.

### Basic matchers

- `match_char<char C>`: Matches a single character `C`.
- `match_string<fixed_string {S}>`: Matches an exact string `S`.
- `match_space_like`: Matches whitespace characters (space, tab, newline).
- `match_identifier`: Matches an alphanumeric sequence (identifier).

### Rule Composition

- `tuple_rule<Rules...>`: Sequence of rules to be provided in order.  
  This rule can be added to an instance of a rule by using the `+` operator.
- `or_rule<Rules...>`: Rule tried in order, at least one must be matching.  
  This rule can be added to an instance of a rule by using the `|` operator.
- `list_rule<Rule>`: Matches zero or more occurrences of `Rule`.
- `repeat<int N, Rule>`: Repeat N times the provided `Rule`.

Rules can be composed using overloaded operators, every rule must inherit from `comsable_rule` to ensure that
composability accross avery rules:

- **Sequence (`+`)**: `RuleA + RuleB` matches `RuleA` followed by `RuleB`. (using `tuple_rule`)
- **Alternation (`|`)**: `RuleA | RuleB` matches `RuleA` or `RuleB` (choice). (using `or_rule`)
- **optional (`~`)**: `!RuleA` matches `RuleA` or not (choice). (using `or_rule`)

Example:

```cpp
// Matches either "TRUE" or "FALSE"
auto boolean_rule = match_string<fixed_string{"TRUE"}>{} | match_string<fixed_string{"FALSE"}>{};
```

### Optional matcher

- `may_rule<Rule>`: Matches zero or one of the provided `Rule`.

> This uses the or_rule behind the hood and thus falls
> under [the performance consideration](#avoiding-tuple_rule-in-or_rule) stated below. The main thing to note is: it is
> expensive to use the may_rule with a tuple_rule.

This rule can be added to an instance of a rule by using the `~` operator.

## Provided Helpers

- `fil::copa::match_semicol`: Matches ';' char
- `fil::copa::match_comma`: Matches ',' char
- `fil::copa::match_if` : Matches "if" string
- `fil::copa::match_while`: Matches "while" string
- `fil::copa::match_space_like`: Matches space like as defined in the C
  standard ([std::isspace](https://en.cppreference.com/w/cpp/string/byte/isspace))

Some are helping common composition:

- `fil::copa::parenthesis_wrapped<...>>` : Matches the rule (...) if around parenthesis `(` rule `)`
- `fil::copa::square_wrapped<...>` : Matches the rule (...) if around square brackets `[` rule `]`
- `fil::copa::angle_wrapped<...>` : Matches the rule (...) if around angle brackets `<` rule `>`

## Important consideration Considerations

### Avoiding `tuple_rule` in `or_rule`

**Avoid for performance reason**

While it is syntactically possible to use (sequences created with the `+` operator) as alternatives within an `or_rule`,
this pattern is **not recommended** due to performance implications.  
When a `tuple_rule` is used as an alternative in an `or_rule` the parser must create a complete copy of the convertor
state before attempting to match that alternative. This is necessary to ensure proper rollback semantics: if the
`tuple_rule` fails during matching, the parser needs to restore the convertor to its previous state before trying the
next alternative.

**Example to avoid:**

```c++
// INEFFICIENT: tuple_rule inside or_rule requires convertor copies
auto rule = (match_string<"ABC">{} + match_identifier{}) 
          | (match_string<"DEF">{} + match_identifier{});
```

This pattern triggers expensive deep copies of the convertor whenever backtracking is needed. For better performance,
consider it.
> This consideration is particularly important in performance-critical parsing scenarios or when dealing with large
> inputs.

**Avoid it for behavioral reasons:**

When using `or_rule` (the `|` operator) with a `tuple_rule` (the `+` operator) that contains multiple parsing rules with
side effects, a critical issue emerges: side effects from partially matched rules are not rolled back when the overall
alternative fails.

It is advised to do the following to avoid `or_rule` of `tuple_rule`

- Flattening the alternatives: Combine non-overlapping prefixes into a single rule when possible
- Restructuring the grammar: Rearrange rules to avoid using parser rule instead
- Using lookahead patterns: Design your grammar to make early decisions before committing to longer sequences. Using a
  keyword to separate different possibilities is a good solution.

## Mapping to AST

To map parsed values to your `ast_object`, use the `member` template:

- `member<&Class::field>`: Assigns the matched value to the specified field.
- If the field is a `std::vector`, `member` will automatically `push_back` the value.
- If the field is a setter method `void set_field(Value)`, it will call that method.

## Integrating with Readers

The `parse` function takes a `reader`. The library provides `fil::buffer_reader` for in-memory strings and is designed
to work with `fil::file_reader` for disk-based parsing.

```cpp
#include <fil/file/file_reader.hh>
// ...
fil::file_reader reader(std::filesystem::path("input.txt"));
auto result = fil::copa::parse(grammar, std::move(reader));
```

# Copa Reader

A **reader** in Copa is an abstraction that provides sequential access to input data. Whether you're parsing from memory
buffers, files, network streams, or custom data sources, implementing a reader allows Copa to work seamlessly with your
data source.

## Reader Concept

A reader is responsible for:

1. **Sequentially accessing data** - Providing bytes one at a time
2. **Cursor management** - Tracking the current position in the input
3. **Backtracking support** - Allowing the parser to revert to previous states
4. **Peek operations** - Looking ahead without consuming data

## Core Requirements

Your custom reader class must implement the following interface:

| Method            | Return Type                   | Description                                |
|-------------------|-------------------------------|--------------------------------------------|
| `next_byte()`     | `std::optional<std::uint8_t>` | Returns and consumes the next byte         |
| `previous_byte()` | `std::optional<std::uint8_t>` | Returns and un-consumes the previous byte  |
| `peek()`          | `std::optional<std::uint8_t>` | Returns the next byte without consuming it |
| `reader_cursor()` | `std::size_t`                 | Returns the current cursor position        |

## The Shallow Copy Concept

### What is Shallow Copy?

When a rule fails (e.g., in an `or_rule`), the parser must backtrack. Readers implement this via a `shallow_copy`
specialization, which allows the parser to save the current state (cursor) without duplicating the underlying data.
This is done for performance reason in order to avoid unecessary copy of the data.
If a shallow copy is not implemented. A normal copy of the reader would be made (which could imply a deepcopy or a copy
that has the read cursor badly set)

### Why is it Important?

Imagine parsing with an `or_rule` that tries multiple alternatives. For each alternative, the parser needs to save the
reader's state. If the alternative fails, it restores the reader to the saved state and tries the next alternative.

**Without shallow copy**: Every backtracking attempt would require a full copy of your data (extremely inefficient for
large files).

**With shallow copy**: Only the cursor position is copied, while the actual data remains shared. This is safe because:

- The original reader stays alive during the entire parse operation
- All shallow copies are destroyed after parsing completes
- No modifications are made to the underlying data

### Implementing Shallow Copy

Specialize the `fil::shallow_copy` template for your reader type:

```c++
// specialization of the shallow_copy
template<> 
struct shallow_copy<YourReaderType> {
     // Create a shallow copy with shared data and copied cursor YourReaderType shallow;

    static constexpr auto copy(const YourReaderType& object) {
        //... implement a copy that does not imply a full data copy
        // easiest way is to declare shallow_copy as a friend of your reader
    }
    
    static constexpr void assign(YourReaderType& object, YourReaderType&& other) {
       // Handle move assignment (usually just cursor update)
    }
};

```

## Implementation Steps

1. **Create a reader class** that manages access to your data source
2. **Implement all required interface methods** for sequential access
3. **Track cursor position** accurately for backtracking
4. **Specialize `fil::shallow_copy`** to enable efficient state saving
5. **Verify the concept** using `static_assert` with `copa::reader<YourType>`

