# File Reader

FiL provides a specialized file reader optimized for fast read-only access using file streams. It's ideal for reading
large files while maintaining low memory overhead.

## Features

- Fast read-only access to specific lines.
- Chunk-based reading via buffers.
- Modern C++ iterator interface.
- Lazy reading of lines (or custom-defined chunks).

## Basic Usage

The `file_reader` returns views over its internal buffer, which makes it very efficient.

```cpp
#include <fil/file/file_reader.hh>
#include <iostream>
#include <string_view>

int main() {
    fil::file_reader reader("large_file.txt");

    // Iterate through lines using standard iterator
    for (auto line : reader) {
        std::cout << line << std::endl;
    }

    return 0;
}
```

## Custom Chunking

You can define your own chunking logic by providing a custom callback.

```cpp
// Example: read file chunk by chunk (e.g., until a custom delimiter)
fil::file_reader reader("large_file.txt");

// You can specify a custom callback to define how to identify chunks/lines
reader.set_delimiter('\n'); // Default is newline
```
