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

#ifndef FIL_DECOPA_MATCHER_HH
#define FIL_DECOPA_MATCHER_HH

#include <string>

#include "fil/copa/member.hh"
#include "fil/copa/production.hh"
#include "fil/copa/rule.hh"
#include "fil/meta/static_string.hh"

#include <print>

namespace fil::copa {

template<fixed_string Str, member_type Mem = member_noop>
struct match_string : composable_rule {
    static_assert(!Str.empty(), "String of match char must be non empty");

    using return_type = std::string;

    static constexpr match_result match(auto& ctx, std::uint8_t c, std::uint32_t = 0) {
        if (Str[ctx.idx.back()++] == c) {
            if (ctx.idx.back() >= Str.size()) {
                ctx.convertor(Mem {}, ctx.current_token);
                return match_result::SUCCESS;
            }
            return match_result::CONTINUE;
        }
        ctx.idx.back() = 0;
        return match_result::FAILURE;
    }

    template<typename Type>
    static constexpr void value(Mem::member_type& obj, Type&& value) {
        member(obj, std::forward<Type>(value));
    }
};

template<char C>
struct match_char : composable_rule {
    using return_type = char;

    static constexpr match_result match(auto&, std::uint8_t c, std::uint32_t = 0) {
        return c == C ? match_result::SUCCESS : match_result::FAILURE;
    }

    template<typename Type, member_type Mem>
    static constexpr void value(Type&& value, Mem member) {
        value.*member = C;
    }
};

struct match_space_like : composable_rule {
    using return_type = char;
    static constexpr match_result match(auto&, std::uint8_t c, std::uint32_t = 0) {
        return std::isspace(c) ? match_result::SUCCESS : match_result::FAILURE;
    }
};

template<member_type Mem = member_noop, rule auto Separator = match_space_like {}>
struct match_identifier : composable_rule {
    using return_type = std::string;
    static constexpr match_result match(auto& ctx, std::uint8_t c, std::uint32_t depth = 0) {
        if (std::isalnum(c)) {
            const auto peek = ctx.reader->peek();
            if (!peek.has_value() || Separator.match(ctx, peek.value(), depth) == match_result::SUCCESS) {
                ctx.convertor(Mem {}, ctx.current_token);
                ctx.current_token = {};
                return match_result::SUCCESS;
            }
            return match_result::CONTINUE;
        }
        return match_result::FAILURE;
    }
};

template<production Prod, member_type Mem = member_noop>
struct match_parser : composable_rule {
    using return_type = Prod::ast_object;

    static constexpr match_result match(auto& ctx, std::uint8_t c, std::uint32_t = 0) {

        details_::matcher_ctx ctx_m_parser {ctx.reader, Prod::convertor()};
        ctx_m_parser.current_token = ctx.current_token;

        auto res = do_parse(ctx_m_parser, Prod {});
        if (!res) {
            return match_result::FAILURE;
        }

        ctx.convertor(Mem {}, std::move(res).value());
        ctx.current_token = {};

        return match_result::SUCCESS;
    }
};

} // namespace fil::copa

#endif // FIL_DECOPA_MATCHER_HH
