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

#ifndef FIL_RULE_HH
#define FIL_RULE_HH

#include <print>

#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "error.hh"
#include "fil/copa/sink.hh"
#include "fil/meta/shallow_copy.hh"

#include <algorithm>

namespace fil::copa {
enum class match_result {
    SUCCESS,
    CONTINUE,
    FAILURE
};

template<typename T>
concept reader =                    //
    std::is_move_assignable_v<T> && //
    requires(T reader_) {
        { reader_.peek() } -> std::convertible_to<std::optional<std::uint8_t>>;
        { reader_.next_byte() } -> std::convertible_to<std::optional<std::uint8_t>>;
        { reader_.previous_byte() } -> std::convertible_to<std::optional<std::uint8_t>>;
        { reader_.reader_cursor() } -> std::convertible_to<std::size_t>;
    };

namespace details_ {

struct reader_noop {
    constexpr std::optional<std::uint8_t> next_byte() { return std::nullopt; }
    constexpr std::optional<std::uint8_t> previous_byte() { return std::nullopt; }
    constexpr std::optional<std::uint8_t> peek() { return std::nullopt; }
    constexpr std::size_t reader_cursor() { return 0; }
};
static_assert(reader<reader_noop>, "reader_noop must follow the reader concept");

template<reader Reader, typename Convertor>
struct rule_ctx {
    using reader_type    = Reader;
    using convertor_type = Convertor;

    Reader* reader;
    Convertor* convertor;

    std::vector<uint16_t> idx {0};
    std::string current_token;

    bool is_main_parser = false;

    error_stack err_stack; //!< current stack of error that occurred

    void increase_depth() { idx.push_back(0); }

    void decrease_depth() { idx.pop_back(); }
};

} // namespace details_

template<typename T>
concept rule = requires {
    typename T::result_type;
    requires std::default_initializable<typename T::result_type>;

    {             //
        T::match( //
            std::declval<details_::rule_ctx<details_::reader_noop, sink::convertor_noop<int>>&>(), std::uint8_t {}, std::uint32_t {})
    } -> std::convertible_to<match_result>;

    {             //
        T::match( //
            std::declval<details_::rule_ctx<details_::reader_noop, sink::convertor_noop<int>>&>(), std::uint8_t {})
    } -> std::convertible_to<match_result>;
};

template<rule... Ts>
struct or_rule;

template<rule... Ts>
struct tuple_rule {
    static constexpr auto size = sizeof...(Ts);

    static_assert(size >= 2, "Match concat must have at least 2 elements");

    using result_type = std::tuple<typename Ts::result_type...>;

    template<rule O>
    constexpr rule auto operator+(const O&) {
        return tuple_rule<Ts..., O> {};
    }

    template<rule O>
    constexpr rule auto operator|(const O&) {
        return or_rule<tuple_rule<Ts...>, O> {};
    }

    static constexpr match_result match(auto& ctx, std::uint8_t c, std::uint32_t depth = 0) {
        match_result current = match_result::FAILURE;

        auto process = [&current, &ctx, c, depth, i = 0]<rule T0>() mutable -> bool {
            if (depth < ctx.idx.size() && i++ == ctx.idx[depth]) {
                if ((ctx.idx.size() - 1) == depth) {
                    ctx.increase_depth();
                }

                current = T0::match(ctx, c, depth + 1);

                if (current == match_result::SUCCESS) {
                    ++ctx.idx[depth];
                }
                if (current != match_result::CONTINUE) {
                    ctx.decrease_depth();
                }
                return true;
            }
            return false;
        };

        ((process.template operator()<Ts>()) || ...);

        if (current == match_result::SUCCESS) {
            return ctx.idx[depth] == size ? match_result::SUCCESS : match_result::CONTINUE;
        }

        return current;
    }

  private:
    std::size_t idx_ = 0;
};

namespace details_ {

template<typename Result>
std::expected<Result, error_stack> do_parse_rule(auto& ctx, const rule auto& formula, const rule auto& ignore);

struct match_space_like { //@todo remove
    using result_type = char;
    static constexpr match_result match(auto&, std::uint8_t c, std::uint32_t = 0) {
        return std::isspace(c) ? match_result::SUCCESS : match_result::FAILURE;
    }
};

template<typename>
struct is_tuple_rule_impl : std::false_type {};

template<rule... Rs>
struct is_tuple_rule_impl<tuple_rule<Rs...>> : std::true_type {};

template<typename Rule>
concept is_tuple_rule = is_tuple_rule_impl<Rule>::value;

} // namespace details_

template<rule... Ts>
struct or_rule {
    static constexpr auto size = sizeof...(Ts);

    static_assert(size >= 2, "Match concat must have at least 2 elements");

    using result_type = std::tuple<typename Ts::result_type...>;

    template<rule O>
    constexpr rule auto operator+(const O&) {
        return tuple_rule<or_rule<Ts...>, O> {};
    }

    template<rule O>
    constexpr rule auto operator|(const O&) {
        return or_rule<Ts..., O> {};
    }

    template<reader Reader, typename Convertor>
    static constexpr match_result match(details_::rule_ctx<Reader, Convertor>& ctx, std::uint8_t, std::uint32_t = 0) {

        auto process = [&ctx]<rule Rule>() -> bool {
            auto shallow_reader = shallow_copy<Reader>::copy(*ctx.reader);
            auto* convertor     = ctx.convertor;

            // if tuple, make a copy of the convertor for re-assignment at the end in case of error
            // this is an inefficient path; it is not recommended to do or_rule with tuples_rule as the rollback in case of error is costly
            std::unique_ptr<Convertor> convertor_copy = nullptr;
            if constexpr (details_::is_tuple_rule<Rule>) {
                convertor_copy = std::make_unique<Convertor>(*ctx.convertor);
                convertor      = convertor_copy.get();
            }

            details_::rule_ctx ctx_or {
                .reader        = &shallow_reader,
                .convertor     = convertor,
                .current_token = ctx.current_token,
            };

            ctx_or.reader->previous_byte(); // go back a character as we went forward before starting or
            ctx_or.current_token.pop_back();

            auto res = details_::do_parse_rule<typename Convertor::value_type>(ctx_or, Rule {}, details_::match_space_like {});
            if (!res) {
                ctx.err_stack.push(std::move(res).error());
                return false;
            }

            ctx.current_token = {};
            if constexpr (details_::is_tuple_rule<Rule>) {
                *ctx.convertor = std::move(*ctx_or.convertor);
            }
            shallow_copy<Reader>::assign(*ctx.reader, std::move(*ctx_or.reader));
            return true;
        };

        const bool success = ((process.template operator()<Ts>()) || ...);
        return success ? match_result::SUCCESS : match_result::FAILURE;
    }

  private:
    std::size_t idx_ = 0;
};

template<rule Rhs, rule Lhs>
using pair_rule = tuple_rule<Rhs, Lhs>;

template<rule R>
struct may_rule;

struct composable_rule {
    template<rule Self, rule O>
    constexpr rule auto operator+(this Self&&, const O&) {
        return pair_rule<Self, O> {};
    }
    template<rule Self, rule O>
    constexpr rule auto operator|(this Self&&, const O&) {
        return or_rule<Self, O> {};
    }
    template<rule Self>
    constexpr rule auto operator~(this Self&&) {
        return may_rule<Self> {};
    }
};

//! eof file match, if the matcher is actually called. It means that the file is not ended as a character has been read
struct eof_rule : composable_rule {
    using result_type = int;
    static constexpr match_result match(auto&, std::uint8_t, std::uint32_t = 0) { return match_result::FAILURE; }
};
static constexpr auto eof = eof_rule {};

struct match_space_like : composable_rule {
    using result_type = char;
    static constexpr match_result match(auto&, std::uint8_t c, std::uint32_t = 0) {
        return std::isspace(c) ? match_result::SUCCESS : match_result::FAILURE;
    }
};
static constexpr auto space_like = match_space_like {};

namespace details_ {

template<rule Rule>
constexpr bool shall_eof_be_success(const Rule&) {
    if constexpr (std::is_same_v<Rule, eof_rule>) {
        return true;
    } else {
        return false;
    }
}

template<rule... Rs>
constexpr bool shall_eof_be_success(const tuple_rule<Rs...>&) {
    static constexpr auto size = sizeof...(Rs);

    using last_rule_type = Rs...[size - 1];
    return shall_eof_be_success(last_rule_type {});
}

template<rule Rule>
constexpr bool strict_eof_check_(const Rule& r) {
    return shall_eof_be_success(r);
}

template<rule... Rs>
constexpr bool strict_eof_check_(const tuple_rule<Rs...>& r) {
    return shall_eof_be_success(r);
}

template<rule... Rs>
constexpr bool strict_eof_check_(const or_rule<Rs...>& r) {
    auto any_rules_is_eof = []<std::size_t... Is>(std::index_sequence<Is...>) { //
        return (std::is_same_v<Rs...[Is], eof_rule> || ...);
    };

    static constexpr auto size = sizeof...(Rs);
    return any_rules_is_eof(std::make_index_sequence<size>());
}

template<rule... Rs>
constexpr bool shall_eof_be_success(const or_rule<Rs...>&) {
    auto any_rules_is_eof = []<std::size_t... Is>(std::index_sequence<Is...>) { //
        return (details_::strict_eof_check_(Rs... [Is] {}) || ...);
    };

    static constexpr auto size = sizeof...(Rs);
    return any_rules_is_eof(std::make_index_sequence<size>());
}

/**
 * every rule at the exception of the tuple rules must have their back idx set to 0 if ended successfully.
 * @note rules playing with depth decrease the depth of the context when in success which result in this truth
 */
constexpr std::size_t rule_idx_value_success(const auto&) { return 0; }

template<rule... Rs>
constexpr std::size_t rule_idx_value_success(const tuple_rule<Rs...>&) {
    return sizeof...(Rs);
}

struct may_rule_not_present_matcher : composable_rule {
    using result_type = bool;
    static constexpr match_result match(auto& ctx, std::uint8_t, std::uint32_t = 0) {
        // as the rule is called, it means no other rules have been successfully completed, the may_rule rollback the read
        ctx.reader->previous_byte();
        return match_result::SUCCESS;
    }
};

template<std::size_t N, rule R, rule... Rs>
struct rule_array_impl : rule_array_impl<N - 1, R, R, Rs...> {}; //!< Recursive case: add another copy of R to Rs

template<rule R, rule... Rs>
struct rule_array_impl<0, R, Rs...> : tuple_rule<R, Rs...> {};   //!< Base case: when N is 0, we need to stop the recursion

} // namespace details_

template<rule R>
struct may_rule : or_rule<R, details_::may_rule_not_present_matcher> {};

template<rule Rule, std::size_t N>
using rule_array = details_::rule_array_impl<N - 1, Rule>;

/**
 * Creates a rule that repeats the given rule a specified number of times.
 *
 * @tparam N The number of repetitions to apply to the rule.
 * @tparam R The rule type to be repeated, which must conform to the rule concept.
 * @param instance An instance of the rule type to be repeated.
 * @return A rule that represents the composition of the given rule repeated N times in sequence.
 */
template<std::size_t N, rule R>
constexpr rule auto repeat([[maybe_unused]] const R& instance) {
    static_assert(N > 0, "cannot have a repeat 0");
    return rule_array<R, N> {};
}

} // namespace fil::copa

#endif // FIL_RULE_HH
