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

#ifndef FIL_DESCPA_H
#define FIL_DESCPA_H

#include "fil/file/file_reader.hh"

namespace fil::descpa
{

namespace details_
{

    class context
    {
        std::uint32_t    curr_rule_idx_ = 0;
        std::uint32_t    position_      = 0;
        std::uint32_t    line_          = 0;
        std::string_view source_name_;
    };

    class lexer
    {
    public:
        explicit lexer(file_reader input) : input_(std::move(input)) {}

        std::string_view next_lexeme(const auto& formula) {}

    private:
        file_reader input_;
    };

} // namespace details_

template <char C>
struct match_char
{
    constexpr bool match(details_::context& ctx, std::uint8_t c)
    {
        return c == C;
    }
};

template <typename T>
concept production = requires(const T& elem) {
    { elem->rule() };
    { elem->produce() };
};

auto parse(production auto& prod, details_::lexer& lexer)
{
    lexer.next_lexeme(prod.rule());
}

auto parse(production auto&& prod, file_reader input)
{
    details_::lexer lexer(std::move(input));

    return parse(prod, lexer);
}

} // namespace fil::descpa

#endif // FIL_DESCPA_H
