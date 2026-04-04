# copa (COmbinator PArser)

`copa` is a C++26 header-only combinator-based parser library. It allows you to build complex recursive descent parsers
by composing simple rules and mapping them directly to C++ structures (AST objects).

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

The following example parse a string of format : "CMD {command_name} ON {target} {options} "

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

- `match_char<char C>`: Matches a single character `C`.
- `match_string<fixed_string {S}>`: Matches an exact string `S`.
- `match_identifier`: Matches an alphanumeric sequence (identifier).
- `match_space_like`: Matches whitespace characters (space, tab, newline).
- `list_rule<Rule>`: Matches zero or more occurrences of `Rule`.
- `may_rule<Rule>`: Matches zero or one of the provided `Rule`.
- `repeat<int N, Rule>`: Repeat N times the provided `Rule`.
- `tuple_rule<Rules...>`: Sequence of rules to be provided in order.
- `or_rule<Rules...>`: Rule tried in order, at least one must be matching.
- `eof`: Matches the end of the input.

## Rule Composition

Rules can be composed using overloaded operators:

- **Sequence (`+`)**: `RuleA + RuleB` matches `RuleA` followed by `RuleB`. (using `tuple_rule`)
- **Alternation (`|`)**: `RuleA | RuleB` matches `RuleA` or `RuleB` (choice). (using `or_rule`)

Example:

```cpp
// Matches either "TRUE" or "FALSE"
auto boolean_rule = match_string<fixed_string{"TRUE"}>{} | match_string<fixed_string{"FALSE"}>{};
```

## Performance Considerations

### Avoiding `tuple_rule` in `or_rule`

While it is syntactically possible to use (sequences created with the `+` operator) as alternatives within an , this
pattern is **not recommended** due to performance implications. `tuple_rule` `or_rule`
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
consider:

- Flattening the alternatives: Combine non-overlapping prefixes into a single rule when possible
- Restructuring the grammar: Rearrange rules to avoid using parser rule instead
- Using lookahead patterns: Design your grammar to make early decisions before committing to longer sequences

This consideration is particularly important in performance-critical parsing scenarios or when dealing with large
inputs.

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
