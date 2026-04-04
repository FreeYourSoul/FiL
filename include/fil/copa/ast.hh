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

#ifndef FIL_AST_BASE_HH
#define FIL_AST_BASE_HH

#include "fil/copa/matcher.hh"
#include "fil/meta/static_string.hh"

namespace fil::copa {

template<fixed_string RightWrap, rule Content, fixed_string LeftWrap>
using wrapped = tuple_rule<match_string<RightWrap>, Content, match_string<LeftWrap>>;

template<rule Content>
using parenthesis_wrapped = wrapped<fixed_string {"("}, Content, fixed_string {")"}>;

template<rule Content>
using bracket_wrapped = wrapped<fixed_string {"{"}, Content, fixed_string {"}"}>;

template<rule Content>
using square_wrapped = wrapped<fixed_string {"["}, Content, fixed_string {"]"}>;

template<rule Content>
using angle_wrapped = wrapped<fixed_string {"<"}, Content, fixed_string {">"}>;

using match_if           = match_string<fixed_string {"if"}>;
using match_while        = match_string<fixed_string {"while"}>;
using match_comma        = match_char<','>;
using match_semicol      = match_char<';'>;
using match_double_point = match_char<':'>;

static constexpr auto semicol      = match_semicol {};
static constexpr auto comma        = match_comma {};
static constexpr auto double_point = match_double_point {};

} // namespace fil::copa

#endif // FIL_AST_BASE_HH
