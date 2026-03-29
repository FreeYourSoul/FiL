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

#include "fil/copa/copa.hh"

namespace fil::copa {

class buffer_reader {

  public:
    explicit constexpr buffer_reader(std::string&& buffer)
        : buffer_(std::move(buffer)) {}

    explicit constexpr buffer_reader(std::span<char> buffer)
        : buffer_(buffer.begin(), buffer.end()) {}

    constexpr std::optional<std::uint8_t> next_byte() {
        if (cursor_ >= buffer_.size()) {
            return std::nullopt;
        }
        return buffer_[cursor_++];
    }

    constexpr std::optional<std::uint8_t> peek() const {
        if (buffer_.empty()) {
            return std::nullopt;
        }
        return buffer_[cursor_];
    }

  private:
    std::string buffer_;
    std::size_t cursor_ = 0;
};

static_assert(reader<buffer_reader>, "buffer_reader must be a reader compatible with descpa");

} // namespace fil::copa

#endif // FIL_BUFFER_READER_HH
