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

#include <expected>

#include "fil/copa/error.hh"
#include "fil/copa/production.hh"
#include "fil/copa/rule.hh"

namespace fil::copa {

namespace details_ {

/**
 * @tparam Prod production with extra requirements to provide ignore() returning a rule of character to ignore
 * @return rule defined by the production Prod::ignore static member function
 */
template<production Prod>
requires requires {
    { Prod::ignore() } -> rule;
}
rule auto retrieve_ignore_rules(const Prod&) {
    return Prod::ignore();
}

/**
 * @brief default ignore rules return match like as being ignored in any provided grammar
 * @return a rule to ignore space like (\n, \t, ' ', etc…) @see std::isspace
 */
rule auto retrieve_ignore_rules(const auto&) { return match_space_like {}; }

template<typename Result>
std::expected<Result, error_parsing> do_parse_rule(auto& ctx, const rule auto& formula, const rule auto& ignore) {

    auto result = match_result::CONTINUE;
    while (result == match_result::CONTINUE) {
        const auto c = ctx.reader->next_byte();
        if (!c.has_value()) {
            if (ctx.idx.back() == 0 && (ctx.is_main_parser || shall_eof_be_success(formula)))
                return ctx.convertor->value();
            return std::unexpected(error_parsing {
                .current_token = ctx.current_token,
                .error_brief   = "parsing didn't finish properly",
            });
        }

        if (ignore.match(ctx, c.value()) == match_result::SUCCESS) {
            continue;
        }

        ctx.current_token += static_cast<char>(c.value());

        result = formula.match(ctx, c.value());
    }
    if (result == match_result::FAILURE) {
        return std::unexpected(error_parsing {
            .current_token = ctx.current_token,
            .error_brief   = "An error occurred in do_parse_rule",
        });
    }

    return ctx.convertor->value();
}

template<reader Reader, typename Convertor, production Prod>
std::expected<typename Prod::ast_object, error_parsing> do_parse(rule_ctx<Reader, Convertor>& ctx, const Prod& prod) {
    const rule auto formula = prod.rules();
    const rule auto ignore  = details_::retrieve_ignore_rules(prod);

    return do_parse_rule<typename Prod::ast_object>(ctx, formula, ignore);
}

template<reader Reader>
class parser {
  public:
    explicit constexpr parser(Reader input)
        : input_(std::move(input)) {}

    constexpr auto parse(const production auto& prod) {
        auto convertor = prod.convertor();
        rule_ctx ctx {
            .reader         = &input_,
            .convertor      = &convertor,
            .is_main_parser = true,
        };
        return details_::do_parse(ctx, prod);
    }

  private:
    Reader input_;
};

} // namespace details_

/**
 * @brief Parses input data according to a grammar production.
 * *  @details This function serves as the main entry point for parsing operations. It creates an internal
 *  parser instance and delegates the actual parsing work to it. The function processes input byte-by-byte,
 *  matching against the rules defined in the production's grammar until completion.
 *
 *  **Important Note on `constexpr` Annotation:**
 *  While this function is marked `constexpr`, it can only be evaluated at runtime in practical use cases when using
 *  a reader that can be instantiated at compile time.
 *  For instance, the `file_reader` parameter relies on file I/O operations (streams, filesystem access) which
 *  are inherently non-constexpr.
 *
 *  @tparam Production A type satisfying the `production` concept.
 *  @param prod The grammar production that defines parsing rules and result construction.
 *              Passed as a non-const reference to allow the parser to access the production's convertor.
 *
 *  @tparam Reader A type satisfying the `reader` concept.
 *  @param input An rvalue reference to a `reader` object. The reader is moved into an internal
 *               parser instance. This parameter enables efficient resource management and ensures
 *               the reader cannot be reused after the parse operation.
 *
 *  @return The result of parsing, obtained from `convertor().value()`. The return type is the ast object of the production
 *
 *  @example
 *  @code
 *  struct MyGrammar {
 *      struct Result {
 *          std::string parsed_value;
 *      };
 *
 *      static constexpr auto rules() {
 *          return match_string<fixed_string{"keyword"}> {};
 *      }
 *
 *      static constexpr auto convertor() {
 *          return sink::aggregator<Result> {};
 *      }
 *  };
 *
 *  fil::file_reader reader{std::filesystem::path("input.txt")};
 *  MyGrammar grammar;
 *  auto result = parse(grammar, std::move(reader));
 *  @endcode
 *
 *  @see fil::descpa::details_::parser for internal parsing implementation
 *  @see fil::descpa::rule for grammar rule concept requirements
 *  @see fil::descpa::production for production concept requirements
 */
constexpr auto parse(production auto& prod, reader auto&& input) {
    details_::parser p(std::forward<decltype(input)>(input));
    return p.parse(prod);
}

} // namespace fil::copa

#endif // FIL_DESCPA_H
