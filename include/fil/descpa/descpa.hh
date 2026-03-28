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

#include <array>
#include <cstdint>
#include <expected>
#include <string_view>
#include <vector>

#include "fil/descpa/ast.hh"
#include "fil/descpa/member.hh"
#include "fil/file/file_reader.hh"

namespace fil::descpa {
namespace sink {
struct convertor_noop;
}

namespace details_ {
template<typename Convertor>
struct matcher_ctx;
}

enum class match_result {
    SUCCESS,
    CONTINUE,
    FAILURE
};

template<typename T>
concept rule = requires {
    typename T::return_type;
    requires std::default_initializable<typename T::return_type>;

    { //
        T::match(std::declval<details_::matcher_ctx<sink::convertor_noop>&>(), std::uint8_t {}, std::uint32_t {})
    } -> std::convertible_to<match_result>;

    { //
        T::match(std::declval<details_::matcher_ctx<sink::convertor_noop>&>(), std::uint8_t {})
    } -> std::convertible_to<match_result>;
};

template<typename T>
concept production = requires {
    { T::rules() } -> rule;
    { T::convertor() };
};

namespace details_ {

struct context {
    std::uint32_t line     = 0;
    std::uint32_t position = 0;
    std::string_view source_name;
};

template<typename Convertor>
struct matcher_ctx {
    Convertor convertor;
    std::vector<uint16_t> idx {0};
    std::string current_token; //!< @todo transform into a view

    void increase_depth() { idx.push_back(0); }

    void decrease_depth() { idx.pop_back(); }
};

class parser {
  public:
    explicit parser(file_reader input)
        : input_(std::move(input)) {}

    auto parse(const production auto& prod) {
        const rule auto formula = prod.rules();

        matcher_ctx ctx {prod.convertor()};
        match_result result = match_result::CONTINUE;
        while (result == match_result::CONTINUE) {
            const auto c = input_.next_byte();
            if (!c.has_value()) {
                return ctx.convertor.value();
            }

            ctx.current_token += static_cast<char>(c.value());

            result = formula.match(ctx, c.value());
        }
        return ctx.convertor.value();
    }

  private:
    file_reader input_;
};

} // namespace details_

template<rule... Ts>
struct tuple_rule {
    static constexpr auto size = sizeof...(Ts);

    static_assert(size >= 2, "Match concat must have at least 2 elements");

    using return_type = std::tuple<typename Ts::return_type...>;

    template<rule O>
    constexpr rule auto operator+(const O&) {
        return tuple_rule<Ts..., O> {};
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

template<rule Rhs, rule Lhs>
using pair_rule = tuple_rule<Rhs, Lhs>;

struct composable_rule {
    template<rule Self, rule O>
    constexpr rule auto operator+(this Self&&, const O&) {
        return pair_rule<Self, O> {};
    } // namespace fil::descpa
};

template<std::size_t N>
struct fixed_string {
    std::array<char, N> data_;

    explicit constexpr fixed_string(const char str[N]) {
        for (std::size_t i = 0; i < N; ++i)
            data_[i] = str[i];
    }

    constexpr char operator[](std::size_t i) const { return data_[i]; }
    [[nodiscard]] constexpr std::size_t size() const { return data_.size(); }
    [[nodiscard]] constexpr bool empty() const { return data_.empty(); }
};

template<std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N - 1>;

template<fixed_string Str, member_type Mem = member_noop>
struct match_string : composable_rule {
    static_assert(!Str.empty(), "String of match char must be non empty");

    using return_type = std::string;

    static constexpr match_result match(auto& ctx, std::uint8_t c, [[maybe_unused]] std::uint32_t depth = 0) {
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

    static constexpr match_result match(auto&, std::uint8_t c, [[maybe_unused]] std::uint32_t depth = 0) {
        return c == C ? match_result::SUCCESS : match_result::FAILURE;
    }

    template<typename Type, member_type Mem>
    static constexpr void value(Type&& value, Mem member) {
        value.*member = C;
    }
};

auto parse(production auto& prod, file_reader&& input) {
    details_::parser p(std::move(input));
    return p.parse(prod);
}

} // namespace fil::descpa

#endif // FIL_DESCPA_H
