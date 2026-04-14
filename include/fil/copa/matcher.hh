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

#include "fil/copa/copa.hh"
#include "fil/copa/member.hh"
#include "fil/meta/static_string.hh"

namespace fil::copa {

template<fixed_string Str, member_type Mem = member_noop>
struct match_string : composable_rule {
    static_assert(!Str.empty(), "String of match char must be non empty");

    using result_type = std::string;

    static constexpr match_result match(auto& ctx, std::uint8_t c, std::uint32_t = 0) {
        if (Str[ctx.idx.back()++] == c) {
            if (ctx.idx.back() >= Str.size()) {
                ctx.convertor->operator()(Mem {}, ctx.current_token);
                ctx.current_token = {};
                return match_result::SUCCESS;
            }
            return match_result::CONTINUE;
        }
        ctx.idx.back() = 0;
        return match_result::FAILURE;
    }

    template<typename Type>
    static constexpr void value(Mem::member_type& obj, Type&&) {
        member(obj, Str.to_string());
    }
};

template<char C, member_type Mem = member_noop>
struct match_char : composable_rule {
    using result_type = char;

    static constexpr match_result match(auto& ctx, std::uint8_t c, std::uint32_t = 0) {
        if (c == C) {
            ctx.convertor->operator()(Mem {}, static_cast<char>(c));
            ctx.current_token = {};
            return match_result::SUCCESS;
        }
        return match_result::FAILURE;
    }
};

template<member_type Mem = member_noop>
struct match_identifier : composable_rule {
    using result_type = std::string;
    static constexpr match_result match(auto& ctx, std::uint8_t c, std::uint32_t = 0) {
        if (std::isalnum(c)) {
            const auto peek = ctx.reader->peek();
            if (!peek.has_value() || !std::isalnum(peek.value())) {
                ctx.convertor->operator()(Mem {}, ctx.current_token);
                ctx.current_token = {};
                return match_result::SUCCESS;
            }
            return match_result::CONTINUE;
        }
        return match_result::FAILURE;
    }
};

template<member_type Mem = member_noop, //
         auto Conversion =
             [](const std::string& token) {
                 try {
                     return std::stoi(token, nullptr, 10);
                 } catch (...) {
                     return 0;
                 }
             }>
struct match_number : composable_rule {
    using result_type = decltype(Conversion("0"));

    static_assert(std::is_integral_v<result_type>, "type of a match_number must be an integral type");

    static constexpr match_result match(auto& ctx, std::uint8_t c, std::uint32_t = 0) {
        if (std::isdigit(c)) {
            const auto peek = ctx.reader->peek();
            if (!peek.has_value() || !std::isdigit(peek.value())) {
                ctx.convertor->operator()(Mem {}, Conversion(ctx.current_token));
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
    using result_type = Prod::ast_object;

    static constexpr match_result match(auto& ctx, std::uint8_t c, std::uint32_t = 0) {
        auto convertor = Prod::convertor();

        details_::rule_ctx ctx_m_parser {
            .reader        = ctx.reader,
            .convertor     = &convertor,
            .current_token = ctx.current_token,
        };

        ctx_m_parser.reader->previous_byte(); // go back a character as we went forward before starting or
        ctx_m_parser.current_token.pop_back();

        auto res = do_parse(ctx_m_parser, Prod {});
        if (!res) {
            return match_result::FAILURE;
        }

        ctx.convertor->operator()(Mem {}, std::move(res).value());
        ctx.current_token = {};

        return match_result::SUCCESS;
    }
};

template<rule Rule>
struct list_rule : composable_rule {
    using value_type  = Rule::result_type;
    using result_type = std::vector<value_type>;

    template<reader Reader, typename Convertor>
    static constexpr match_result match(details_::rule_ctx<Reader, Convertor>& ctx, std::uint8_t c, std::uint32_t depth = 0) {
        if ((ctx.idx.size() - 1) == depth) {
            ctx.increase_depth();
        }

        ctx.reader->previous_byte(); // go back a character as we went forward before starting or
        ctx.current_token.pop_back();

        using shallow    = shallow_copy<Reader>;
        auto copy_reader = shallow::copy(*ctx.reader);
        details_::rule_ctx reset_ctx {
            .reader    = &copy_reader,
            .convertor = ctx.convertor,
        };

        ++ctx.idx.back();
        const auto current = details_::do_parse_rule<typename Convertor::value_type>(ctx, Rule {}, match_space_like {});
        --ctx.idx.back();

        ctx.current_token = {};

        if (current) {
            return match_result::CONTINUE;
        }

        ctx.decrease_depth();

        shallow::assign(*ctx.reader, std::move(*reset_ctx.reader));

        return match_result::SUCCESS;
    }
};

} // namespace fil::copa

#endif // FIL_DECOPA_MATCHER_HH
