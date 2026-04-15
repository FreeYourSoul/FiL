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

#include "fil/meta/reader.hh"
#include "fil/meta/shallow_copy.hh"

namespace fil {

/**
 * @class buffer_reader
 * @brief A class for sequentially reading and accessing data from an in-memory buffer.
 *
 * The `buffer_reader` class provides functionality for reading data, one byte at a time,
 * from a string buffer. It supports reading, peeking, and checking whether its instance is a shallow copy of another `buffer_reader`.
 * @note implement fil::shallow_buffer specialization which ensure that the copy doesn't copy the buffer content
 * @see fill::shallow_buffer
 */
class buffer_reader {

    template<typename>
    friend struct shallow_copy;

  public:
    /**
     * @brief wrapper aground the buffer line: used to respect the meta::line_reader concept
     */
    class buffer_line {
      public:
        explicit buffer_line(std::string_view l)
            : line_(l) {}

        [[nodiscard]] std::string_view get() const { return line_; }

      private:
        std::string_view line_;
    };

    explicit constexpr buffer_reader(std::string&& buffer)
        : buffer_(std::move(buffer))
        , buffer_access_(buffer_) {}

    explicit constexpr buffer_reader(std::span<char> buffer)
        : buffer_(buffer.begin(), buffer.end())
        , buffer_access_(buffer_) {}

    buffer_reader(buffer_reader&& other) noexcept
        : buffer_(std::move(other.buffer_))
        , buffer_access_(buffer_)
        , cursor_(other.cursor_) {}

    buffer_reader& operator=(buffer_reader&& other) noexcept {
        buffer_        = std::move(other.buffer_);
        buffer_access_ = std::string_view(buffer_.begin(), buffer_.end());
        cursor_        = other.cursor_;
        return *this;
    }
    buffer_reader(const buffer_reader&)            = default;
    buffer_reader& operator=(const buffer_reader&) = default;

    /**
     * @return buffer cursor
     */
    [[nodiscard]] std::size_t reader_cursor() const { return cursor_; }

    /**
     * @note the buffer cursor progress forward
     * @return the next character of the buffer, if any
     */
    [[nodiscard]] constexpr std::optional<std::uint8_t> next_byte() {
        if (cursor_ >= buffer_access_.size()) {
            return std::nullopt;
        }
        return buffer_access_[cursor_++];
    }

    /**
     * @note the buffer cursor progress backward
     * @return the previous character of the buffer, if any
     */
    constexpr std::optional<std::uint8_t> previous_byte() {
        if (cursor_ <= 0) {
            return std::nullopt;
        }
        return buffer_access_[--cursor_];
    }

    /**
     * @note the buffer cursor doesn't progress forward
     * @return the next character, if any
     */
    [[nodiscard]] constexpr std::optional<std::uint8_t> peek() const {
        if (buffer_access_.empty()) {
            return std::nullopt;
        }
        return buffer_access_[cursor_];
    }

    buffer_line read_line(std::size_t line_nb) {
        std::size_t cursor_begin        = 0;
        std::size_t cursor_end          = 0;
        std::size_t current_line_number = 1;

        for (const char c : buffer_access_) {
            if (current_line_number < line_nb)
                ++cursor_begin;
            if (c == '\n')
                ++current_line_number;
            if (current_line_number > line_nb)
                break;
            ++cursor_end;
        }
        cursor_ = cursor_end;

        return buffer_line {buffer_access_.substr(cursor_begin, cursor_end - cursor_begin)};
    }

    buffer_line next_line() {
        const auto it_next_line = std::ranges::find(buffer_access_.begin(), buffer_access_.end(), '\n');
        const auto size         = buffer_access_.size() - std::distance(it_next_line, buffer_access_.end());
        const auto line         = buffer_access_.substr(cursor_, size);

        cursor_ = std::distance(buffer_access_.begin(), it_next_line);

        return buffer_line {line};
    }

    /**
     * @return true if the buffer is the result of shallow_copy
     */
    [[nodiscard]] constexpr bool is_shallow_copy() const { return buffer_.empty() && !buffer_access_.empty(); }

  private:
    explicit constexpr buffer_reader() = default;

  private:
    std::string buffer_;
    std::string_view buffer_access_;
    std::size_t cursor_ = 0;
};

static_assert(meta::bytes_reader<buffer_reader>, "buffer_reader must be a byte reader");
static_assert(meta::line_reader<buffer_reader>, "buffer_reader must be a line reader");

/**
 * @brief specialization of the shallow_copy making it possible to copy the buffer without copying the buffer.
 */
template<>
struct shallow_copy<buffer_reader> {
    static constexpr auto copy(const buffer_reader& object) {
        buffer_reader shallow;
        shallow.buffer_access_ = object.buffer_access_;
        shallow.cursor_        = object.cursor_;
        return shallow;
    }

    static constexpr auto assign(buffer_reader& object, buffer_reader&& other) { object.cursor_ = other.cursor_; }
};

} // namespace fil

#endif // FIL_BUFFER_READER_HH
