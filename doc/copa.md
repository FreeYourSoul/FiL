# copa (Combinator Parser)

`copa` is a combinator-based parser library. It allows you to build complex recursive descent parsers by composing
rules.

## Basic Usage

The library uses concepts to define rules and productions.

```cpp
#include <fil/copa/copa.hh>
#include <fil/copa/sink.hh>

// Example: checking for a specific character
fil::copa::match_char<'X'> char_x_check;
fil::copa::details_::matcher_ctx<fil::copa::sink::convertor_noop> ctx;

auto res = char_x_check.match(ctx, 'X'); 
// res == fil::copa::match_result::SUCCESS
```

## String Matching

```cpp
// Check for a fixed string
fil::copa::match_string<fil::copa::fixed_string {"CHOCOBO"}> string_check;

// Returns CONTINUE while the string is being matched, and SUCCESS when complete
string_check.match(ctx, 'C'); // CONTINUE
string_check.match(ctx, 'H'); // CONTINUE
// ...
string_check.match(ctx, 'O'); // SUCCESS
```

## Rule Composition

You can compose rules using the `+` operator.

```cpp
// Match "LOL" by composing 'L', 'O', 'L'
auto lol_parser = fil::copa::match_char<'L'>{} + 
                  fil::copa::match_char<'O'>{} + 
                  fil::copa::match_char<'L'>{};

// This creates a tuple_rule that matches the sequence.
```

## Integrating with File Reader

`copa` is designed to work efficiently with `file_reader`.

```cpp
#include <fil/file/file_reader.hh>
// Use fil::parse with a production and a reader
// auto result = fil::copa::parse(production, reader);
```
