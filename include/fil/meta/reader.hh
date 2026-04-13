// MIT License
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

#ifndef FIL_READER_HH
#define FIL_READER_HH

namespace fil::meta {

template<typename T>
concept line_retrieved = requires(const T& t) {
    { t.get() } -> std::convertible_to<std::string_view>;
};

template<typename T>
concept line_reader = requires(T& reader_, std::size_t line_number) {
    { reader_.read_line(line_number) } -> line_retrieved;
    { reader_.next_line() } -> line_retrieved;
    { reader_.reader_cursor() } -> std::convertible_to<std::size_t>;
};

template<typename T>
concept bytes_reader =              //
    std::is_move_assignable_v<T> && //
    requires(T reader_) {
        { reader_.peek() } -> std::convertible_to<std::optional<std::uint8_t>>;
        { reader_.next_byte() } -> std::convertible_to<std::optional<std::uint8_t>>;
        { reader_.previous_byte() } -> std::convertible_to<std::optional<std::uint8_t>>;
        { reader_.reader_cursor() } -> std::convertible_to<std::size_t>;
    };

} // namespace fil::meta

#endif // FIL_READER_HH
