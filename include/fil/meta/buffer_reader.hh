/// MIT License
//
// Copyright (c) 2025 Quentin Balland
// Repository : https://github.com/FreeYourSoul/FiL
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//         of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
//         to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//         copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
//         copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//         AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef FIL_BUFFER_READER_HH
#define FIL_BUFFER_READER_HH

#include <cstdint>
#include <optional>
#include <span>
#include <string>

#include "fil/copa/rule.hh"
#include "fil/meta/shallow_copy.hh"

namespace fil {

class buffer_reader {

    template<typename>
    friend struct shallow_copy;

  public:
    explicit constexpr buffer_reader(std::string&& buffer)
        : buffer_(std::move(buffer))
        , buffer_access_(buffer_) {}

    explicit constexpr buffer_reader(std::span<char> buffer)
        : buffer_(buffer.begin(), buffer.end())
        , buffer_access_(buffer_) {}

    [[nodiscard]] std::size_t get_buffer_cursor() const { return cursor_; }

    [[nodiscard]] constexpr std::optional<std::uint8_t> next_byte() {
        if (cursor_ >= buffer_access_.size()) {
            return std::nullopt;
        }
        return buffer_access_[cursor_++];
    }

    [[nodiscard]] constexpr std::optional<std::uint8_t> peek() const {
        if (buffer_access_.empty()) {
            return std::nullopt;
        }
        return buffer_access_[cursor_];
    }

    [[nodiscard]] constexpr bool is_shallow_copy() const { return buffer_.empty() && !buffer_access_.empty(); }

  private:
    explicit constexpr buffer_reader() = default;

  private:
    std::string buffer_;
    std::string_view buffer_access_;
    std::size_t cursor_ = 0;
};

static_assert(copa::reader<buffer_reader>, "buffer_reader must be a reader compatible with descpa");

} // namespace fil

namespace fil {
template<>
struct shallow_copy<buffer_reader> {
    static constexpr auto operator()(const buffer_reader& object) {
        buffer_reader shallow;
        shallow.buffer_access_ = object.buffer_;
        shallow.cursor_        = object.cursor_;
        return shallow;
    }
};
} // namespace fil

#endif // FIL_BUFFER_READER_HH
