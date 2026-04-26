# File Reader Documentation

The `file_reader` class provides efficient, buffered access to file data with support for byte-level and line-level
reading. It's optimized for reading large files while maintaining low memory overhead by using a fixed-size internal
buffer (1MB by default).

## Table of Contents

- [Overview](#overview)
- [Core Features](#core-features)
- [Basic Usage](#basic-usage)
    - [Creating a File Reader](#creating-a-file-reader)
    - [Line-by-Line Iteration](#line-by-line-iteration)
    - [Byte-Level Reading](#byte-level-reading)
- [Advanced Reading Methods](#advanced-reading-methods)
    - [Reading Specific Lines](#reading-specific-lines)
    - [Reading Until a Condition](#reading-until-a-condition)
- [Block Views and Validity](#block-views-and-validity)
- [Iterator Interface](#iterator-interface)
- [File Information](#file-information)
- [Shallow Copy Optimization](#shallow-copy-optimization)
- [Complete Examples](#complete-examples)
- [Concepts and Traits](#concepts-and-traits)

---

## Overview

`file_reader` is a C++26 streaming file reader designed for efficient, sequential access to file data. Unlike in-memory
readers (e.g., `buffer_reader`), it reads data in configurable chunks (default: 1MB) to handle arbitrarily large files
without loading everything into memory at once.

### Key Design Principles

- **Buffered Reading**: Uses a 1MB buffer by default for efficient I/O operations
- **Lazy Loading**: Data is only loaded when needed
- **Cursor Management**: Maintains position tracking for random access to loaded data
- **Shallow Copy Support**: Enables efficient backtracking in parsing operations
- **View-Based API**: Returns `block_view` objects that safely reference buffer data

---

## Core Features

- **Line-by-line iteration** with standard C++ iterator interface
- **Random line access** via `read_line(line_number)`
- **Byte-level sequential access** with `next_byte()` and `previous_byte()`
- **Predicate-based reading** with `read_until(predicate)`
- **File metadata** (size, path, existence checks)
- **Load tracking** to detect when buffer reloads occur
- **Shallow copy support** for efficient state saving and backtracking (for efficient integraton with `copa parser`)

---

## Basic Usage

### Creating a File Reader

```c++
#include <fil/file/file_reader.hh>
#include <iostream>

int main() {
    // Create a file reader from a file path
    fil::file_reader reader(std::filesystem::path("large_file.txt"));

    // Check if the file exists and is readable
    if (!reader.exists()) {
        std::cerr << "File not found!" << std::endl;
        return 1;
    }

    return 0;
}
```

### Line-by-Line Iteration

The most common usage pattern is iterating through lines in a file:

```c++
#include <fil/file/file_reader.hh>
#include <iostream>

int main() {
    fil::file_reader reader("data.txt");

    // Iterate through all lines
    for (auto line_iter = reader.make_line_iterator(); line_iter != reader.end(); ++line_iter) {
        auto line = *line_iter;
        
        // Check if the line is still valid (important for large files with buffer reloads)
        if (line.is_valid()) {
            std::cout << "Line " << line_iter.line() << ": " << line.get() << std::endl;
        }
    }

    return 0;
}
```

**Note:** Always check `is_valid()` before using a `block_view` from `file_reader`, as the underlying buffer may be
reloaded.

### Byte-Level Reading

For sequential byte-by-byte reading (useful in parsing):

```c++
#include <fil/file/file_reader.hh>
#include <iostream>

int main() {
    fil::file_reader reader("input.bin");

    std::optional<std::uint8_t> byte;
    while ((byte = reader.next_byte()).has_value()) {
        std::cout << static_cast<char>(*byte);
    }

    return 0;
}
```

---

### Reading Specific Lines

Access any line in the file directly (performs a seek to that line):

```c++
#include <fil/file/file_reader.hh>

int main() {
    fil::file_reader reader("data.txt");

    // Jump directly to line 42
    auto line_42 = reader.read_line(42);
    
    if (line_42.is_valid()) {
        std::cout << "Line 42: " << line_42.get() << std::endl;
    }

    return 0;
}
```

**Performance Note:** `read_line()` performs file seeks, making it slower for sequential access. For iteration, use the
iterator interface.

### Reading Until a Condition (String-based Predicate)

Read data from the current position until a predicate condition is met (receiving accumulated `std::string_view`):

```c++
#include <fil/file/file_reader.hh>

int main() {
    fil::file_reader reader("data.txt");

    // Read until we find a closing brace
    auto block = reader.read_until([](std::string_view accumulated) {
        return accumulated.back() == '}';
    });

    if (!block.get().empty()) {
        std::cout << "Block: " << block.get() << std::endl;
    }

    return 0;
}
```

### Reading Until a Condition (Char-based Predicate)

Read data until a predicate on individual characters is satisfied:

```c++
#include <fil/file/file_reader.hh>

int main() {
    fil::file_reader reader("data.txt");

    // Read until we encounter a newline
    auto line = reader.read_until([](char c) {
        return c == '\n';
    });

    if (!line.get().empty()) {
        std::cout << "Line: " << line.get() << std::endl;
    }

    return 0;
}
```

### Custom Minimum Buffer Size

Both `read_until` variants support a `minimum_size` parameter to control when buffer reloading occurs:

```c++
// Read until condition, but only reload if less than 500 bytes remain
auto block = reader.read_until(
    [](std::string_view sv) { return sv.find(";") != std::string_view::npos; },
    500  // minimum_size parameter
);
```

---

## Block Views and Validity

### Understanding `block_view`

A `block_view` is a safe wrapper around a `std::string_view` that points to data in the `file_reader`'s internal buffer.
Since the buffer can be reloaded during reading, views may become invalid.

```c++
struct block_view {
    /**
     * @return true if the referenced data is still valid in the buffer
     * 
     * Returns false if:
     * - The buffer has been reloaded (load_counter has changed)
     * - The reader has been destroyed
     */
    [[nodiscard]] bool is_valid() const;

    /**
     * @return string_view to the block data, or empty if invalid
     * 
     * Always check is_valid() before calling get()
     */
    [[nodiscard]] std::string_view get() const;
};
```

### Best Practices with `block_view`

```c++
#include <fil/file/file_reader.hh>
#include <vector>
#include <string>

int main() {
    fil::file_reader reader("large_file.txt");
    std::vector<std::string> lines;

    // ✓ CORRECT: Copy the data immediately
    for (auto line_iter = reader.make_line_iterator(); line_iter != reader.end(); ++line_iter) {
        auto line = *line_iter;
        if (line.is_valid()) {
            lines.push_back(std::string(line.get()));  // Copy the view to a string
        }
    }

    // ✗ WRONG: Storing the view directly (will become invalid after buffer reload)
    // std::vector<fil::file_reader::block_view> bad_lines;
    // for (auto line_iter = reader.make_line_iterator(); ...) {
    //     bad_lines.push_back(*line_iter);  // Dangerous!
    // }

    return 0;
}
```

---

## Iterator Interface

### Line Iterator

The `file_reader::line_iterator` provides standard C++ iterator semantics for line-based iteration:

```c++
#include <fil/file/file_reader.hh>

int main() {
    fil::file_reader reader("data.txt");

    // Create an iterator starting at line 1
    auto begin = reader.make_line_iterator(1);
    auto end = reader.end();

    // Use with standard algorithms
    int line_count = 0;
    for (auto it = begin; it != end; ++it) {
        ++line_count;
        std::cout << "Line " << it.line() << std::endl;
    }

    return 0;
}
```

### Iterator Operations

```c++
// Create iterator at specific line
auto iter = reader.make_line_iterator(10);

// Get current line data
auto block = *iter;
auto line_num = iter.line();

// Advance to next line
++iter;

// Check for end
if (iter != reader.end()) {
    // More lines to read
}
```

### Range-Based Iteration

Use `file_reader_line` for convenient range-based for loops:

```c++
#include <fil/file/file_reader.hh>

int main() {
    fil::file_reader reader("data.txt");

    // Range-based iteration starting from line 1
    for (const auto& line : fil::file_reader_line(reader, 1)) {
        if (line.is_valid()) {
            std::cout << line.get() << std::endl;
        }
    }

    // Start from line 100
    for (const auto& line : fil::file_reader_line(reader, 100)) {
        if (line.is_valid()) {
            std::cout << line.get() << std::endl;
        }
    }

    return 0;
}
```

---

## File Information

The `file_reader` provides access to file metadata:

```c++
#include <fil/file/file_reader.hh>

int main() {
    fil::file_reader reader("data.txt");

    // File path
    auto path = reader.get_path();
    std::cout << "File: " << path << std::endl;

    // File size in bytes
    std::size_t file_size = reader.size();
    std::cout << "Size: " << file_size << " bytes" << std::endl;

    // Check existence
    if (reader.exists()) {
        std::cout << "File exists" << std::endl;
    }

    // Current position in file stream
    auto file_pos = reader.get_file_cursor();
    
    // Current position in buffer
    std::size_t buffer_pos = reader.get_buffer_cursor();
    
    // Equivalent: reader_cursor() returns buffer position
    std::size_t cursor = reader.reader_cursor();

    // Number of buffer reloads that have occurred
    std::size_t loads = reader.load_counter();
    std::cout << "Buffer loaded " << loads << " times" << std::endl;

    return 0;
}
```

---

## Shallow Copy Optimization

### Why Shallow Copy Matters

When parsing with backtracking (e.g., using `or_rule` in Copa), the parser needs to save reader state. Without shallow
copy optimization, this would require copying the entire file buffer, which is extremely inefficient for large files.

The `file_reader` implements `shallow_copy` to:

- Create a new file stream positioned at the same location
- Copy only the cursor position, not the buffer data
- Maintain reference to the same buffer accessor

---

## Concepts and Traits

### Bytes Reader Concept

The `file_reader` satisfies the `fil::meta::bytes_reader` concept:

```c++
static_assert(fil::meta::bytes_reader<fil::file_reader>);
```

Required interface:

- `next_byte()` → `std::optional<std::uint8_t>`
- `previous_byte()` → `std::optional<std::uint8_t>`
- `peek()` → `std::optional<std::uint8_t>`
- `reader_cursor()` → `std::size_t`

### Line Reader Concept

The `file_reader` satisfies the `fil::meta::line_reader` concept:

```c++
static_assert(fil::meta::line_reader<fil::file_reader>);
```

Allows seamless integration with parsing libraries requiring sequential line access.

---

## Performance Considerations

### Buffer Size

The default buffer size is **1MB** (`READER_BUFFER_SIZE = 1024 * 1024 + 1`). This is optimized for most use cases:

- **Larger files**: Multiple buffer loads, but memory-efficient
- **Smaller files**: An entire file fits in one load
- **Line-heavy parsing**: Good balance between I/O operations and memory

### I/O Strategy

- Buffer reloads happen automatically when reading beyond the current buffer
- Previous unread data is preserved (cursor adjusted on reload)
- File stream position is carefully managed to minimize seeks

### Iterator Overhead

- Line iteration requires scanning for newline characters
- Consider byte-level reading for non-line-oriented formats
- Load counter helps detect when buffer has been reloaded

---

## Troubleshooting

**Q: Why is my `block_view` empty?**

- Check `is_valid()` before using `get()`. The buffer may have been reloaded.

**Q: Can I access a line at the end of a large file efficiently?**

- Use `read_line(line_number)` for random access, but be aware it performs file seeks.

**Q: Is byte-level reading efficient for large files?**

- Yes, it uses the same buffering mechanism as line reading. Buffer reloads are transparent.