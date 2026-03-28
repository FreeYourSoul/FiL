# Algorithms & Datastructures

FiL includes several helper algorithms and datastructures, mostly header-only.

## String Algorithms

FiL provides a set of common string manipulation utilities.

```cpp
#include <fil/algorithm/string.hh>
#include <iostream>
#include <string>

// Example: trim a string
std::string text = "   Hello, World!   ";
fil::trim(text);
// Result: "Hello, World!"
```

Other available string algorithms:

- `to_lower`: Convert string to lowercase.
- `to_upper`: Convert string to uppercase.
- `split`: Split string into a vector of strings based on a delimiter.
- `join`: Join elements of a collection into a single string.
- `starts_with` / `ends_with`: Check if a string has a given prefix/suffix.
- `contains`: Check if a collection contains a given element (found in `fil/algorithm/contains.hh`).

## Boundary Map

The `boundary_map` is a specialized map-like structure for handling value ranges.

```cpp
#include <fil/datastructure/boundary_map.hh>

fil::boundary_map<int, std::string> map;
map.add(10, "Small");
map.add(50, "Medium");
map.add(100, "Large");

// Finds the value corresponding to the largest boundary <= the given key
std::string res = map.get(25); // returns "Small"
res = map.get(75); // returns "Medium"
```

## Random Number Generator (RNG)

A lightweight wrapper for C++ random number generation.

```cpp
#include <fil/datastructure/rng.hh>

fil::rng random;
int value = random.next(1, 100); // random int between 1 and 100
double d_value = random.next_double(); // random double between 0 and 1
```

## Circular Buffer

A fixed-size circular buffer implementation.

```cpp
#include <fil/datastructure/fixed_circular_buffer.hh>

fil::fixed_circular_buffer<int, 10> buffer;
buffer.push_back(1);
buffer.push_back(2);
int top = buffer.pop_front(); // returns 1
```
