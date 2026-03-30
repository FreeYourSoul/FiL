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

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "fil/copa/sink.hh"
#include "fil/meta/shallow_copy.hh"

namespace fil::copa {

enum class match_result {
    SUCCESS,
    CONTINUE,
    FAILURE
};

template<typename T>
concept reader = requires(T reader_) {
    std::is_move_assignable_v<T>;
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
    Reader* reader;
    Convertor* convertor;

    std::vector<uint16_t> idx {0};
    std::string current_token; //!< @todo transform into a view

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

        auto process = [&current, &ctx, c, depth, i = 0]<typename T0>() mutable -> bool {
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

    static constexpr match_result match(auto& ctx, std::uint8_t c, std::uint32_t depth = 0) {
        match_result current = match_result::FAILURE;

        // @todo : execute as a do_parse
        auto process = [&current, &ctx, c, depth, i = 0]<typename T0>() mutable -> bool {
            if (depth < ctx.idx.size() && i++ == ctx.idx[depth]) {
                if ((ctx.idx.size() - 1) == depth) {
                    ctx.increase_depth();
                }

                current = T0::match(ctx, c, depth + 1);

                if (current == match_result::FAILURE) {
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

        if (current == match_result::FAILURE && ctx.idx[depth] == size) {
            return match_result::FAILURE;
        }

        return current;
    }

  private:
    std::size_t idx_ = 0;
};

template<rule Rhs, rule Lhs>
using pair_rule = tuple_rule<Rhs, Lhs>;

struct composable_rule {
    template<rule Self, rule O>
    constexpr rule auto operator+(this Self&&, const O&) {
        return pair_rule<Self, O> {};
    } // namespace fil::descpa
};

struct match_space_like : composable_rule {
    using result_type = char;
    static constexpr match_result match(auto&, std::uint8_t c, std::uint32_t = 0) {
        return std::isspace(c) ? match_result::SUCCESS : match_result::FAILURE;
    }
};

} // namespace fil::copa

#endif // FIL_RULE_HH
