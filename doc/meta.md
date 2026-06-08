# Meta Utilities

The `fil::meta` namespace contains several metaprogramming helpers, reader abstractions, and utility types used
throughout the library.

## Reader Abstractions

These are used primarily by the `copa` parser but can be used independently for efficient data reading.

### Buffer Reader

`fil::buffer_reader` provides a reader interface over a contiguous memory buffer (like `std::string` or `std::vector`).

```cpp
#include <fil/meta/buffer_reader.hh>

std::string data = "Hello, World!";
fil::buffer_reader reader(std::move(data));

auto byte = reader.next_byte(); // Optional containing 'H'
auto pos = reader.reader_cursor(); // 1
```

## Metaprogramming Helpers

### Type Traits

FiL provides several custom type traits in `include/fil/meta/type_traits.hpp` to help with template metaprogramming.

### Tuple Utilities

Utilities for working with `std::tuple`, including iteration and transformation.

### Visitor Utilities

Helpers for `std::visit` and handling `std::variant`.

## Static String

`fil::static_string` (or `fixed_string` in some contexts) is used to pass string literals as template parameters, which
is essential for the `copa` matcher definitions.

```cpp
#include <fil/meta/static_string.hh>

template<fil::fixed_string S>
void print_string() {
    std::cout << S.value << std::endl;
}

print_string<"Hello">();
```

## Shallow Copy

The `shallow_copy` concept is used to create lightweight copies of objects that share the same underlying data but have
their own state (like a read cursor).

To support `shallow_copy` for a custom type, specialize the `fil::shallow_copy` template:

```cpp
#include <fil/meta/shallow_copy.hh>

template<>
struct fil::shallow_copy<MyType> {
    static MyType copy(const MyType& obj) {
        // Return a copy that shares data
    }
    static void assign(MyType& obj, MyType&& other) {
        // Move assignment
    }
};
```
