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

template<fixed_string Str, mem_or_cb_type Mem = member_noop>
struct match_string : composable_rule {
    static_assert(!Str.empty(), "String of match char must be non empty");

    using result_type = std::string;

    static constexpr match_result match(auto& ctx, std::uint8_t c, std::uint32_t = 0) {
        if (Str[ctx.idx.back()++] == c) {
            if (ctx.idx.back() >= Str.size()) {
                ctx.convertor->operator()(ctx.convertor_ctx, Mem {}, ctx.current_token);
                ctx.current_token = {};
                return match_result::SUCCESS;
            }
            return match_result::CONTINUE;
        }
        ctx.idx.back() = 0;
        return match_result::FAILURE;
    }

    template<typename Type>
    static constexpr void value(auto& obj, Type&&) {
        member(obj, Str.to_string());
    }
};

template<char C, mem_or_cb_type Mem = member_noop>
struct match_char : composable_rule {
    using result_type = char;

    static constexpr match_result match(auto& ctx, std::uint8_t c, std::uint32_t = 0) {
        if (c == C) {
            ctx.convertor->operator()(ctx.convertor_ctx, Mem {}, static_cast<char>(c));
            ctx.current_token = {};
            return match_result::SUCCESS;
        }
        return match_result::FAILURE;
    }
};

/**
 * @brief Matches a sequence of consecutive alphanumeric characters (an identifier).
 *
 * @details `match_identifier` recognizes and extracts identifiers from the input stream.
 * An identifier is a sequence of alphanumeric characters (`[a-zA-Z0-9]+`) that terminates
 * when a non-alphanumeric character is encountered. This rule is commonly used for matching
 * variable names, keywords, symbols, and other word-like tokens in parsing.
 *
 * @tparam Mem  The target member or callback where the matched identifier will be stored.
 *              Defaults to `member_noop` (no operation).
 *              - Use @c fil::copa::member<&ClassName::member> to bind to a `std::string` member
 *              - Use a callback type to process the identifier with custom logic
 *
 * @par Template Members
 * - `result_type`: @c std::string, the type of the matched identifier
 *
 * @note Termination Conditions
 * `match_identifier` completes matching when:
 * - A non-alphanumeric character is encountered
 * - Whitespace is encountered (spaces, tabs, newlines)
 * - Any special character (symbols, punctuation, etc.) is found
 *
 * @par Character Classification
 * The rule uses @c std::isalnum() following the C standardization of classification of character:
 * - Alphanumeric: `a-z`, `A-Z`, `0-9`
 * - Non-alphanumeric: anything else (whitespace, symbols, punctuation)
 *
 * @par Whitespace Handling
 * Whitespace is not consumed by `match_identifier` itself. Instead, whitespace
 * between tokens is handled by the default ignore rules (typically `match_space_like`).
 * This allows seamless integration with multi-rule grammars where whitespace
 * naturally separates tokens.
 *
 * @attention Limitations
 * - Cannot match identifiers containing underscores or hyphens (by default)
 * - Does not distinguish between keywords and regular identifiers
 * - No length restrictions on identifier names
 * - Cannot enforce identifier conventions (e.g., camelCase, snake_case)
 */
template<mem_or_cb_type Mem = member_noop>
struct match_identifier : composable_rule {
    using result_type = std::string;
    static constexpr match_result match(auto& ctx, std::uint8_t c, std::uint32_t = 0) {
        if (std::isalnum(c)) {
            const auto peek = ctx.reader->peek();
            if (!peek.has_value() || !std::isalnum(peek.value())) {
                ctx.convertor->operator()(ctx.convertor_ctx, Mem {}, ctx.current_token);
                ctx.current_token = {};
                return match_result::SUCCESS;
            }
            return match_result::CONTINUE;
        }
        return match_result::FAILURE;
    }
};

template<mem_or_cb_type Mem = member_noop, //
         auto Conversion    = [](const std::string& token) -> int {
             try {
                 return std::stoi(token, nullptr, 10);
             } catch (...) {
                 return 0;
             }
         }>
struct match_number : composable_rule {
    using result_type = std::decay_t<decltype(Conversion("0"))>;
    static_assert(std::is_integral_v<result_type>, "type of a match_number must be an integral type");

    static constexpr match_result match(auto& ctx, std::uint8_t c, std::uint32_t = 0) {
        if (std::isdigit(c)) {
            const auto peek = ctx.reader->peek();
            if (!peek.has_value() || !std::isdigit(peek.value())) {
                ctx.convertor->operator()(ctx.convertor_ctx, Mem {}, Conversion(ctx.current_token));
                ctx.current_token = {};
                return match_result::SUCCESS;
            }
            return match_result::CONTINUE;
        }
        return match_result::FAILURE;
    }
};

/**
 * @brief Embeds a nested grammar production within another grammar rule.
 *
 * @details`match_parser` allows the composition of multiple grammars by treating a complete
 * grammar production as a single matching rule. This enables modular, hierarchical,
 * and recursive grammar definitions.
 *
 * @tparam Prod The grammar that will be matched @c fil::copa::production type to be parsed as a nested rule.
 * @tparam Mem  The target member or callback where the parsed result will be stored.
 *              Defaults to @c fil::copa::member_noop` (which implies no operation to be executed with the parse result).
 *              - Use `@c fil::copa::member<&ClassName::member>` to bind to a member variable
 *              - Use a callback type to process the parsed result with custom logic defined as a concept @c callback_type
 *
 * @par Requirements
 * - `Prod` must be a valid @c fil::copa::production
 * - The nested grammar must be fully defined before @c match_parser is instantiated
 *   (though forward declarations are allowed if instantiation is deferred)
 *
 * @par Parsing Behavior
 * When a `match_parser` is encountered during parsing:
 * 1. A new parser instance is created for the nested grammar `Prod`
 * 2. The parser attempts to match the input against `Prod::rules()`
 * 3. If successful, the resulting `Prod::ast_object` is produced
 * 4. The result is passed to the member/callback specified by `Mem`
 * 5. If parsing fails, the entire match fails and input is not consumed
 *
 * @see @c fil::copa::production
 * @see @c fil::copa::member
 * @see @c fil::copa::sink::aggregator
 * @see @c fil::copa::list_rule
 */
template<typename Prod, mem_or_cb_type Mem = member_noop>
struct match_parser : composable_rule {
    using result_type = Prod::ast_object;

    static constexpr match_result match(auto& ctx, std::uint8_t c, std::uint32_t = 0) {
        static_assert(production<Prod>, "type provided to a match_parser must be a fil::copa::production.");

        auto convertor = Prod::convertor();
        details_::rule_ctx ctx_m_parser {
            .reader        = ctx.reader,
            .convertor     = &convertor,
            .convertor_ctx = ctx.convertor_ctx,
            .current_token = ctx.current_token,
        };

        ctx_m_parser.reader->previous_byte(); // go back a character as we went forward before starting or
        ctx_m_parser.current_token.pop_back();

        auto res = do_parse(ctx_m_parser, Prod {});
        if (!res) {
            return match_result::FAILURE;
        }

        ctx.convertor->operator()(ctx.convertor_ctx, Mem {}, std::move(res).value());
        ctx.current_token = {};

        return match_result::SUCCESS;
    }
};

template<typename Prod, mem_or_cb_type Mem = member_noop>
struct match_production : composable_rule {
    using result_type = Prod::ast_object;

    static constexpr match_result match(auto& ctx, std::uint8_t, std::uint32_t = 0) {
        static_assert(production<Prod>, "type provided to a match_parser must be a fil::copa::production.");

        using shallow = shallow_copy<std::decay_t<decltype(*ctx.reader)>>;
        auto reader   = shallow::copy(*ctx.reader);

        auto parser = details_::parser {reader};
        auto prod   = Prod {};
        auto res    = parser.parse(prod);

        if (!res) {
            return match_result::FAILURE;
        }

        std::println("-------");
        fil::copa::debug::print_ast_tree(res.value());
        std::println("-------");

        shallow::assign(*ctx.reader, std::move(reader));

        ctx.convertor->operator()(ctx.convertor_ctx, Mem {}, std::move(res).value());
        ctx.current_token = {};

        return match_result::SUCCESS;
    }
};

/**
 * @brief Matches zero or more consecutive occurrences of a rule and collects results into a container.
 *
 * @details `list_rule` applies a given rule repeatedly to the input stream until the rule fails to match.
 * All successful matches are accumulated into a `std::vector` of the rule's result type.
 * This rule allows matching variable-length sequences of homogeneous elements without specifying
 * an exact count.
 *
 * @tparam Rule The rule to be applied repeatedly.
 *              Must satisfy the @c fil::copa::rule concept.
 *
 * @note Greedy Zero-or-More Semantics
 * `list_rule` is greedy and accepts zero matches, making it suitable for:
 * - Optional lists that may be empty
 * - Variable-length sequences where the end is determined by a different rule failing
 * - Greedy consumption of input until the pattern no longer matches
 *
 * @attention  Performance Considerations
 * - Each iteration performs a full rule evaluation, which may be expensive for complex rules
 * - Reader state is shallow-copied (see @c fil::shallow_copy for more details) on each iteration for backtracking safety
 * - The result vector grows dynamically as matches are found
 * - Early termination occurs as soon as the rule fails to be matched
 *
 * @par Parsing Behavior
 * The rule proceeds as follows:
 * 1. Attempts to apply `Rule` to the current input position
 * 2. If `Rule` matches successfully, the result is added to the accumulating vector
 * 3. The input position advances past the matched content
 * 4. Step 1 is repeated from the new position
 * 5. When `Rule` fails to match, collection stops
 * 6. Returns `SUCCESS` with the accumulated vector (which may be empty if no matches occurred)
 *
 * @see @c fil::copa::rule
 * @see @c fil::copa::match_parser
 * @see @c fil::copa::sink::aggregator
 * @see @c fil::copa::composable_rule
 */
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

/**
 * @brief helper function to build an instance of a list type
 * @tparam Rule to embed in a list rule
 * @return instance of the provided rule as a list
 * @see @c fil::copa::list_rule
 */
template<rule Rule>
list_rule<Rule> list(const Rule&) {
    return list_rule<Rule> {};
}

} // namespace fil::copa

#endif // FIL_DECOPA_MATCHER_HH
