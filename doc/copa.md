# Copa (DEclarative COmbinatory PArser)

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
- [Important Considerations](#important-considerations)
    - [Avoiding `tuple_rule` in `or_rule`](#avoiding-tuple_rule-in-or_rule)
- [Mapping to AST](#mapping-to-ast)
- [Integrating with Readers](#integrating-with-readers)
- [Copa Reader](#copa-reader)
    - [Reader Concept](#reader-concept)
    - [Core Requirements](#core-requirements)
    - [The Shallow Copy Concept](#the-shallow-copy-concept)
    - [Implementation Steps](#implementation-steps)
- [AST Tree Generator](#ast-tree-generator)
    - [Overview](#overview)
    - [Core Concepts](#core-concepts)
    - [AST Node Structure](#ast-node-structure)
    - [Using ast_tree_generator](#using-ast_tree_generator)
- [How Structural Parsing and AST Construction Interact](#how-structural-parsing-and-ast-construction-interact)
    - [The Callback Mechanism](#the-callback-mechanism)
    - [Shared Convertor Context in match_parser](#shared-convertor-context-in-match_parser)
    - [Isolated Parsing with match_production](#isolated-parsing-with-match_production)
    - [Example: Expression Parser with Operator Precedence](#example-expression-parser-with-operator-precedence)
    - [Worked Example: Parsing "1 + 2 * 3"](#worked-example-parsing-1--2--3)
    - [Parenthesized Subexpressions](#parenthesized-subexpressions)
- [Advanced Features](#advanced-features)

---

## Key Concepts

- **Rule**: A basic building block that matches a part of the input. Rules can be simple (match a character) or
  complex (composed of other rules).
- **Matcher**: A specific implementation of a rule (e.g., `match_char`, `match_string`).
- **Production**: A high-level grammar definition that combines rules with an AST object and a result convertor.
- **Sink (Aggregator)**: Collects results from matchers and populates the AST object.

---

## Basic Usage

To use `copa`, you define a **Production**. A production is a struct or class that defines:

1. `ast_object`: The type that will hold the parsing results.
2. `rules()`: A static method returning the composed rules.
3. `convertor()`: A static method returning an aggregator for the `ast_object`.

### Example: A Simple Command Parser

The following example parses a string of format: "CMD {command_name} ON {target} {options} "

```c++
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

---

## Available Matchers

`copa` provides several built-in matchers in the `fil::copa` namespace:

> **Greedy Matcher**: Copa design is based on greedy matching. If a matcher can match multiple times, it will match
> until it can't anymore. For instance, this means that it is impossible to match a list of integers followed by an
> integer of a specific value. As the list would consume that integer.  
> The design of the parser must take that into account.

### Basic matchers

- `match_char<char C>`: Matches a single character `C`.
- `match_string<fixed_string {S}>`: Matches an exact string `S`.
- `match_space_like`: Matches whitespace characters (space, tab, newline).
- `match_identifier`: Matches an alphanumeric sequence (identifier).
- `match_number`: Matches numeric values.

### Rule Composition

- `tuple_rule<Rules...>`: Sequence of rules to be provided in order.  
  This rule can be added to an instance of a rule by using the `+` operator.
- `or_rule<Rules...>`: Rule tried in order, at least one must be matching.  
  This rule can be added to an instance of a rule by using the `|` operator.
- `list_rule<Rule>`: Matches zero or more occurrences of `Rule`.
- `repeat<int N, Rule>`: Repeat N times the provided `Rule`.

### Grammar Composition

Copa allows embedding a full grammar production into another rule. This is essential for modularizing complex grammars
or implementing recursion.

- `match_parser<Production, Member>`: Embeds a nested `Production` as a rule. It uses the `Production`'s rules and its
  own convertor instance, but **shares the same convertor context** (e.g., the tree being built) with the parent parser.
  This is essential for multi-level precedence where different rules contribute to the same AST structure.
- `match_production<Production, Member>`: Performs a complete, **isolated** parsing cycle using a fresh
  `fil::copa::parser` instance. It has its own independent convertor and context. The result of this production is
  passed as a single completed object to the parent's convertor. This is most likely required when you need to "reset"
  the parsing context, such as when passing from a general aggregator to a `tree_generator`, or for handling
  parenthesized sub-expressions where the sub-tree must be completed before being attached as a single leaf to the outer
  tree. (recursion of grammar is another example)

Rules can be composed using overloaded operators. Every rule must inherit from `composable_rule` to ensure
composability across all rules:

- **Sequence    (`+`)**: `RuleA + RuleB` matches `RuleA` followed by `RuleB` (using `tuple_rule`).
- **Alternation (`|`)**: `RuleA | RuleB` matches `RuleA` or `RuleB` (choice) (using `or_rule`).
- **Optional    (`~`)**: `~RuleA` matches `RuleA` or nothing (using `or_rule`).

Example:

```c++
// Matches either "TRUE" or "FALSE"
auto boolean_rule = match_string<fixed_string{"TRUE"}>{} | match_string<fixed_string{"FALSE"}>{};
```

### Optional matcher

- `may_rule<Rule>`: Matches zero or one of the provided `Rule`.

> This uses the or_rule behind the hood and thus falls
> under [the performance consideration](#avoiding-tuple_rule-in-or_rule) stated below. The main thing to note is: it is
> expensive to use the may_rule with a tuple_rule.

This rule can be added to an instance of a rule by using the `~` operator.

---

## Provided Helpers

- `fil::copa::match_semicol`: Matches `;` char
- `fil::copa::match_comma`: Matches `,` char
- `fil::copa::match_if`: Matches `if` string
- `fil::copa::match_while`: Matches `while` string
- `fil::copa::match_space_like`: Matches space like as defined in the C
  standard ([std::isspace](https://en.cppreference.com/w/cpp/string/byte/isspace))

Some helping common compositions:

- `fil::copa::parenthesis_wrapped<...>`: Matches the rule wrapped in parentheses `(` rule `)`
- `fil::copa::bracket_wrapped<...>`: Matches the rule wrapped in curly brackets `{` rule `}`
- `fil::copa::square_wrapped<...>`: Matches the rule wrapped in square brackets `[` rule `]`
- `fil::copa::angle_wrapped<...>`: Matches the rule wrapped in angle brackets `<` rule `>`
- `fil::copa::apostrophed_wrapped<...>`: Matches the rule wrapped in quotes `"` rule `"`

Some utility functions to use those wrappers as values (and thus combining with `|`, `+` or `~`):

- `fil::copa::parenthesised(const Rule auto rule) -> fil::copa::parenthesis_wrapped<Rule>`:
- `fil::copa::apostrophed(const Rule auto rule) -> fil::copa::apostrophed_wrapped<Rule>`:

---

## Important Considerations

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
consider restructuring.

> This consideration is particularly important in performance-critical parsing scenarios or when dealing with large
> inputs.

**Avoid it for behavioral reasons:**

When using `or_rule` (the `|` operator) with a `tuple_rule` (the `+` operator) that contains multiple parsing rules with
side effects, a critical issue emerges: side effects from partially matched rules are not rolled back when the overall
alternative fails.

It is advised to do the following to avoid `or_rule` of `tuple_rule`:

- Flattening the alternatives: Combine non-overlapping prefixes into a single rule when possible
- Restructuring the grammar: Rearrange rules to avoid using parser rule instead
- Using lookahead patterns: Design your grammar to make early decisions before committing to longer sequences. Using a
  keyword to separate different possibilities is a good solution.

---

## Mapping to AST

To map parsed values to your `ast_object`, you can use either the `member` template or a custom **callback**:

### Using `member`

- `member<&Class::field>`: Assigns the matched value to the specified field.
- If the field is a `std::vector`, `member` will automatically `push_back` the value.
- If the field is a setter method `void set_field(Value)`, it will call that method.

### Using Callbacks

You can also provide a custom callback instead of a member pointer. A callback is any invocable type (struct with
`operator()`, lambda, or function pointer) that accepts the matched value.

#### Value-returning Callbacks

If the callback returns a value, that value is assigned to the `ast_object` (the object being aggregated).

```c++
struct MyGrammar {
    using ast_object = std::string;

    struct my_callback {
        std::string operator()(std::string value) const {
            return "PREFIX_" + value;
        }
    };

    static constexpr auto rules() {
        return fil::copa::match_identifier<my_callback>{};
    }
    static constexpr auto convertor() {
        return fil::copa::sink::aggregator<ast_object>{};
    }
};
```

In this case, the `aggregator` will store the string returned by `my_callback`.

#### Void Callbacks

If the callback returns `void`, it is executed for its side effects, and the `ast_object` remains unchanged (or is
default-initialized).

```c++
struct MyGrammar {
    using ast_object = int;
    struct void_callback {
        void operator()(std::string value) const {
            std::cout << "Parsed: " << value << std::endl;
        }
    };

    static constexpr auto rules() {
        return fil::copa::match_identifier<void_callback>{};
    }
    static constexpr auto convertor() {
        return fil::copa::sink::aggregator<ast_object>{};
    }
};
```

---

## Integrating with Readers

The `parse` function takes a `reader`. The library provides `fil::buffer_reader` for in-memory strings and is designed
to work with `fil::file_reader` for disk-based parsing.

```c++
#include <fil/file/file_reader.hh>
// ...
fil::file_reader reader(std::filesystem::path("input.txt"));
auto result = fil::copa::parse(grammar, std::move(reader));
```

---

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
specialization, which allows the parser to save the current state (cursor) without duplicating the underlying data. This
is done for performance reasons to avoid unnecessary copy of the data.
If a shallow copy is not implemented, a normal copy of the reader would be made (which could imply a deepcopy or a copy
that has the read cursor badly set).

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
    // Create a shallow copy with shared data and copied cursor
    static constexpr auto copy(const YourReaderType& object) {
        //... implement a copy that does not imply a full data copy
        // easiest way is to declare shallow_copy as a friend of your reader
    }

    static constexpr void assign(YourReaderType& object, YourReaderType&& other) {
        // Handle move assignment (usually just cursor update)
    }
};
```

---

# AST Tree Generator

## Overview

The `ast_tree_generator` is a specialized convertor sink in Copa designed to build **abstract syntax trees (AST)** with
proper **operator precedence handling**. Unlike the simple `aggregator` convertor that directly assigns values to struct
members, `ast_tree_generator` constructs a hierarchical tree structure suitable for expression parsing, especially when
handling operators with different precedence levels.

### When to Use

- **Expression parsers** with operators of varying precedence (e.g., calculator with `+`, `-`, `*`, `/`)
- **Binary operator trees** where parentheses or precedence rules determine the tree structure
- **Recursive grammar rules** where multiple precedence levels are defined separately

### Understanding Precedence and Context Sharing

The power of `ast_tree_generator` comes from how it interacts with `match_parser` and `match_production`:

- **`match_parser`** is used to combine different precedence levels (e.g., `level_1_grammar` and `level_2_grammar`).
  Because it shares the convertor context, the tree being built in `level_2` is the SAME tree being built in `level_1`.
  This allows the `ast_tree_generator` to perform tree rotations across level boundaries.
- **`match_production`** is used to start a FRESH tree. When you encounter a parenthesized expression, you want to build
  that entire sub-tree independently and then treat the result as a single value (a leaf) in the outer expression.
  `match_production` provides this isolation.

### When NOT to Use

- Simple data structures that don't require tree hierarchies
- Non-expression-based grammars (use `aggregator` instead)

---

## Core Concepts

### AST Node Structure

An `ast_node` is a binary tree node that represents an operation or value in an expression. It consists of:

```c++
template<std::invocable<std::string> auto CallbackOp>
struct ast_node {
    using operand_type = std::invoke_result_t<decltype(CallbackOp), std::string>;

    operand_type value;                          // The operator or value
    std::variant<std::shared_ptr<ast_node>, 
                 std::string, int, char> lhs;   // Left-hand side child
    std::variant<std::shared_ptr<ast_node>, 
                 std::string, int, char> rhs;   // Right-hand side child

    struct operand : callback<CallbackOp> {};    // Callback for operators
    struct leaf : callback<[](const std::string& value) { 
        return value; 
    }> {};                                        // Callback for operands/leaves
};
```

### Key Components

1. **`value`**: Stores the operator or result at this node (transformed via the `operand` callback)
2. **`lhs` (left)**: The left child - can be another AST node, string, int, or char
3. **`rhs` (right)**: The right child - can be another AST node, string, int, or char
4. **`operand` callback**: Transforms matched operator tokens into meaningful values
5. **`leaf` callback**: Marks operand/literal values (the leaves of the tree)

### Precedence Handling

The `ast_tree_generator` takes a **precedence level** parameter:

```c++
explicit constexpr ast_tree_generator(std::uint32_t precedence = 0)
    : precedence_(precedence) {}
```

- **Higher precedence operators** (e.g., `*` and `/` have precedence 2) bind tighter than lower ones
- **Lower precedence operators** (e.g., `+` and `-` have precedence 1) bind looser
- The precedence determines how the tree is restructured during parsing

---

## Using ast_tree_generator

### Step 1: Define Your Callback

Create a callback that transforms operator tokens into meaningful values:

```c++
enum class op : int {
    INVALID,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE
};

using ast_node = fil::copa::ast_node<[](const std::string& token) -> op {
    if (token == "+") return op::PLUS;
    if (token == "-") return op::MINUS;
    if (token == "*") return op::MULTIPLY;
    if (token == "/") return op::DIVIDE;
    return op::INVALID;
}>;
```

### Step 2: Define Grammar Rules for Each Precedence Level

Create separate grammar rules for each precedence level, each with its own `ast_tree_generator`:

```c++
// Lowest precedence: addition and subtraction
struct level_1_grammar {
    using ast_object = ast_node;

    static constexpr auto rules() {
        return fil::copa::match_string<fil::fixed_string {"+"}, ast_node::operand> {} 
             | fil::copa::match_string<fil::fixed_string {"-"}, ast_node::operand> {};
    }
    
    static constexpr auto convertor() { 
        return fil::copa::sink::ast_tree_generator<ast_node> {1};  // precedence = 1
    }
};

// Higher precedence: multiplication and division
struct level_2_grammar {
    using ast_object = ast_node;

    static constexpr auto rules() {
        return fil::copa::match_string<fil::fixed_string {"*"}, ast_node::operand> {} 
             | fil::copa::match_string<fil::fixed_string {"/"}, ast_node::operand> {};
    }
    
    static constexpr auto convertor() { 
        return fil::copa::sink::ast_tree_generator<ast_node> {2};  // precedence = 2
    }
};
```

### Step 3: Define Operand/Leaf Rules

Define rules for operands (leaves of the tree):

```c++
struct base_grammar {
    using ast_object = ast_node;

    static constexpr auto rules() {
        return fil::copa::match_number<ast_node::leaf> {}
             | fil::copa::parenthesised(
                 fil::copa::match_parser<calculator_grammar, ast_node::leaf> {});
    }

    static constexpr auto convertor() { 
        return fil::copa::sink::ast_tree_generator<ast_node> {0};  // precedence = 0 (base level)
    }
};
```

### Step 4: Compose All Levels

Combine all precedence levels into a complete grammar:

```c++
struct calculator_grammar {
    using ast_object = ast_node;

    static constexpr auto rules() {
        return fil::copa::list_rule<fil::copa::or_rule<
            fil::copa::match_parser<base_grammar>,      // Try base first (operands)
            fil::copa::match_parser<level_1_grammar>,   // Then level 1 operators
            fil::copa::match_parser<level_2_grammar>    // Then level 2 operators
        >> {};
    }

    static constexpr auto convertor() { 
        return fil::copa::sink::ast_tree_generator<ast_node> {0};
    }
};
```

---

# How Structural Parsing and AST Construction Interact

Structural parsing (rules and matchers) and AST construction (the convertor) are two separate concerns in Copa.
Rules define **what** to match; the convertor defines **how to build** the result from what was matched. This section
explains the internal data flow that connects them.

## The Callback Mechanism

Every matcher holds two template parameters: the string/character pattern to match, and a **member or callback** (`Mem`)
that determines what happens with the matched value.

When a matcher successfully completes, it immediately invokes the convertor:

```c++
ctx.convertor->operator()(ctx.convertor_ctx, Mem{}, matched_value);
```

The `Mem` type controls how the value is routed:

| `Mem` type                                                 | Convertor behavior                                                      |
|------------------------------------------------------------|-------------------------------------------------------------------------|
| `member<&Struct::field>`                                   | Assigns the matched value to a struct field (used with `aggregator`)    |
| Callback type (e.g. `ast_node::leaf`, `ast_node::operand`) | Invokes the callback to modify the AST (used with `ast_tree_generator`) |
| `member_noop` (default)                                    | Does nothing — the value is discarded                                   |

The **convertor context** (`ctx_extension`) persists across all matcher firings for a given parsing level. For
`aggregator` this context is irrelevant. For `ast_tree_generator` it holds the tree nodes currently being
assembled (specifically the `current_node`, `previous_node`, and `prev_prec` used for rotations).

## Shared Convertor Context in match_parser

This is the critical integration point for expression parsing with multiple precedence levels.

When `match_parser<Prod>` runs, it:

1. Creates a **new convertor** from `Prod::convertor()` — the nested grammar's own convertor instance
2. Passes the **same `ctx_extension` pointer** that belongs to the outer grammar's convertor
3. Runs `Prod`'s rules to completion using that shared context

Because the `ctx_extension` is shared, every `leaf` and `operand` callback fired by any of the nested grammars all
write into the **same tree being assembled** by the outer grammar. The different `precedence` values on each nested
convertor instance are what control how the tree is restructured. This is why `match_parser` is used to chain together
different operator levels (e.g., addition rules and multiplication rules).

## Isolated Parsing with match_production

`match_production<Prod, Mem>` provides a completely different integration model:

1. It **shallow-copies** the current reader (cursor only, underlying data is shared)
2. Creates a **fresh standalone parser** with its own convertor and **its own fresh `ctx_extension`**
3. Parses the input as an independent sub-expression
4. Passes the **result** (the fully completed `ast_object`) to the parent convertor via `Mem{}`
5. Synchronizes the parent reader's cursor to the position after the consumed input

This isolation means the sub-expression is parsed as a self-contained unit. When passed to the outer grammar's convertor
via a `leaf` callback, the entire sub-tree becomes a single opaque node — exactly the right behavior for parenthesized
expressions. Using `match_production` effectively "resets" the precedence logic because it starts a new tree from
scratch.

## Parenthesized Subexpressions

Parentheses are handled by `match_production` in `base_grammar::rules()`:

```c++
constexpr auto base_grammar::rules() {
    return fil::copa::match_number<ast_node::leaf> {}
         | fil::copa::parenthesised(
               fil::copa::match_production<expression_grammar, ast_node::leaf> {});
}
```

When the parser encounters `(`:

1. `match_production` shallow-copies the reader and spawns a fresh `expression_grammar` parser
2. That parser builds its own complete sub-tree (e.g., `{+,lhs:1,rhs:2}` for `(1 + 2)`)
3. The result is passed to `base_grammar`'s convertor as a **leaf** via `ast_node::leaf`
4. The `leaf` handler wraps it in a `shared_ptr<ast_node>` and stores it in `tmp_node->lhs`

From the outer tree's perspective, `(1 + 2)` is just a leaf — identical in position to a plain number. This is what
gives parentheses their grouping power: they reset precedence context completely.

---

## Example: Expression Parser with Operator Precedence

Here's a complete calculator example that parses and builds an AST for mathematical expressions:

```c++
#include <fil/copa/copa.hh>
#include <fil/copa/sink.hh>
#include <fil/meta/buffer_reader.hh>

enum class op : int {
    INVALID, PLUS, MINUS, MULTIPLY, DIVIDE
};

using ast_node = fil::copa::ast_node<[](const std::string& token) -> op {
    if (token == "+") return op::PLUS;
    if (token == "-") return op::MINUS;
    if (token == "*") return op::MULTIPLY;
    if (token == "/") return op::DIVIDE;
    return op::INVALID;
}>;

// Multiplication and Division (precedence 2)
struct level_2_grammar {
    using ast_object = ast_node;

    static constexpr auto rules() {
        return fil::copa::match_string<fil::fixed_string {"*"}, ast_node::operand> {} 
             | fil::copa::match_string<fil::fixed_string {"/"}, ast_node::operand> {};
    }

    static constexpr auto convertor() { 
        return fil::copa::sink::ast_tree_generator<ast_node> {2};
    }
};

// Addition and Subtraction (precedence 1)
struct level_1_grammar {
    using ast_object = ast_node;

    static constexpr auto rules() {
        return fil::copa::match_string<fil::fixed_string {"+"}, ast_node::operand> {} 
             | fil::copa::match_string<fil::fixed_string {"-"}, ast_node::operand> {};
    }

    static constexpr auto convertor() { 
        return fil::copa::sink::ast_tree_generator<ast_node> {1};
    }
};

// Operands and Base Grammar (precedence 0)
struct base_grammar {
    using ast_object = ast_node;

    //! defined below as recursion over calculator_grammar requires the class definition to be defined before the function definition
    static constexpr auto rules(); 

    static constexpr auto convertor() { 
        return fil::copa::sink::ast_tree_generator<ast_node> {0};
    }
};

// Complete Grammar
struct calculator_grammar {
    using ast_object = ast_node;

    static constexpr auto rules() {
        return fil::copa::list_rule<fil::copa::or_rule<
            fil::copa::match_parser<base_grammar>,
            fil::copa::match_parser<level_1_grammar>,
            fil::copa::match_parser<level_2_grammar>
        >> {};
    }

    static constexpr auto convertor() { 
        return fil::copa::sink::ast_tree_generator<ast_node> {0};
    }
};

constexpr auto base_grammar::rules() {
    return fil::copa::match_number<ast_node::leaf> {}
         | fil::copa::parenthesised(
             fil::copa::match_production<calculator_grammar, ast_node::leaf> {});
}

int main() {
    std::string input = "16 * (1337 + 42)";
    fil::buffer_reader reader(std::move(input));
    
    auto result = fil::copa::parse(calculator_grammar{}, std::move(reader));
    
    if (result) {
        // The resulting AST structure represents the expression
        // with proper precedence handling:
        //        *
        //       / \
        //      16  +
        //         / \
        //      1337 42
        
        std::cout << "Parse successful!" << std::endl;
    }
}
```

## Worked Example: Parsing "1 + 2 * 3"

Using the calculator grammar (level_1: prec=1 for `+`/`-`, level_2: prec=2 for `*`/`/`, base: prec=0 for operands):

```
Shared ctx_extension state progression:

① match_parser<base_grammar> matches "1":
    base convertor (prec=0) fires leaf(1)
    → tmp_node = {lhs: 1}
    ctx: {tmp_node:{lhs:1}, current:null, previous:null, prev_prec:0}

② match_parser<level_1_grammar> matches "+":
    level_1 convertor (prec=1) fires operand("+")
    → first operator: current_node = previous_node = {op:+, lhs:1}
    ctx: {tmp_node:null, current:{+,lhs:1}, previous:{+,lhs:1}, prev_prec:1}

③ match_parser<base_grammar> matches "2":
    base convertor (prec=0) fires leaf(2)
    → tmp_node = {lhs: 2}
    ctx: {tmp_node:{lhs:2}, current:{+,lhs:1}, previous:{+,lhs:1}}

④ match_parser<level_2_grammar> matches "*":
    level_2 convertor (prec=2) fires operand("*")
    → prec(2) >= prev_prec(1): append to right of previous_node
    → previous_node->rhs = {op:*, lhs:2}; previous_node = &{*,lhs:2}
    ctx: {tmp_node:null, current:{+,lhs:1,rhs:{*,lhs:2}}, previous:{*,lhs:2}, prev_prec:2}

⑤ match_parser<base_grammar> matches "3":
    base convertor (prec=0) fires leaf(3)
    → tmp_node = {lhs: 3}
    ctx: {tmp_node:{lhs:3}, current:{+,lhs:1,rhs:{*,lhs:2}}, previous:{*,lhs:2}}

⑥ End of input → convertor.value() called:
    → previous_node->rhs = tmp_node->lhs  →  {*,lhs:2,rhs:3}

Result:
        +
       / \
      1   *
         / \
        2   3
```

The tree is built **in a single left-to-right pass**; no post-processing or separate rewriting step is needed.

The same precedence logic produces a different shape for `"2 * 3 + 4"` — when a lower-precedence operator
arrives after a higher-precedence one (step ④ in reverse), the algorithm rotates the tree:

```
After matching "2 * 3":
    ctx: {current:{*,lhs:2}, previous:{*,lhs:2}, prev_prec:2, tmp_node:{lhs:3}}

Matching "+", prec(1) < prev_prec(2): ROTATE
    previous_node->rhs = tmp_node->lhs   →  {*,lhs:2,rhs:3}
    tmp_node->lhs      = current_node    →  {+,lhs:{*,2,3}}
    current_node = previous_node = tmp_node

After matching "4" and calling value():
        +
       / \
      *   4
     / \
    2   3
```

---

## Advanced Features

### Custom Node Value Types

By default, `ast_node` stores leaves as `std::string`, `int`, or `char`. You can extend this with additional types
by providing extra template arguments:

```c++
struct MyFloat {};

using ast_node = fil::copa::ast_node<
    [](const std::string& token) -> op { /* ... */ },
    MyFloat,         // added to the node_type variant
    double           // also added
>;

// The node_type variant now includes: monostate, shared_ptr<ast_node>, string, int, char, MyFloat, double
```

This is useful when your grammar needs to store typed leaf values beyond the built-in set.

### Debugging: to_string()

`ast_node` provides a `to_string()` method for visualizing the tree structure during development:

```c++
auto result = fil::copa::parse(expression_grammar{}, std::move(reader));
if (result) {
    // Prints a human-readable representation of the entire AST
    std::println("{}", result.value().to_string());
}
```

The output format is also accessible via `fil::to_string(result.value())` if you specialize
`fil::to_string` for your operator enum, as shown in the calculator test.

### Designing the Operator Callback

The `CallbackOp` lambda passed to `ast_node` is invoked once per operator token. Keep it lightweight — it is called
during parsing, not as a post-processing step. A `switch` on a pre-tokenized enum is the typical pattern:

```c++
using ast_node = fil::copa::ast_node<[](const std::string& token) -> op {
    if (token == "+") return op::PLUS;
    if (token == "-") return op::MINUS;
    if (token == "*") return op::MULTIPLY;
    if (token == "/") return op::DIVIDE;
    return op::INVALID;    // always handle the unknown case
}>;
```

> Avoid any operation that could fail or throw inside the callback: parsing does not have a recovery mechanism at that
> point.

---

## Performance Notes

- **Precedence Handling**: Tree restructuring is O(depth) for each operator, typically very efficient
- **Memory**: Uses `std::shared_ptr` for node management; consider memory usage with very deep trees
- **Callback Overhead**: The operator callback is invoked once per operator; keep it lightweight
- **Variant Storage**: The use of `std::variant` for `lhs` and `rhs` provides flexibility but has minor runtime overhead

