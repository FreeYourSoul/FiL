#include "ast.hh"

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <print>
#include <random>
#include <string>

#include "fil/descpa/descpa.hh"
#include "fil/file/temporary.hh"

TEST_CASE("descpa basic tests", "[descpa]") {

    fil::descpa::details_::matcher_ctx<fil::descpa::ast::convertor_noop> ctx;

    SECTION("test char") {

        fil::descpa::match_char<'X'> char_x_check;

        CHECK(char_x_check.match(ctx, 'X') == fil::descpa::match_result::SUCCESS);
        CHECK(char_x_check.match(ctx, 'Y') == fil::descpa::match_result::FAILURE);
        CHECK(char_x_check.match(ctx, 'x') == fil::descpa::match_result::FAILURE);
    }
    SECTION("test string") {

        fil::descpa::match_string<fil::descpa::fixed_string {"CHOCOBO"}> string_check;

        SECTION("Success") {
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 0);
            CHECK(string_check.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 1);
            CHECK(string_check.match(ctx, 'H') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 2);
            CHECK(string_check.match(ctx, 'O') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 3);
            CHECK(string_check.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 4);
            CHECK(string_check.match(ctx, 'O') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 5);
            CHECK(string_check.match(ctx, 'B') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 6);
            CHECK(string_check.match(ctx, 'O') == fil::descpa::match_result::SUCCESS);
            CHECK(ctx.idx.back() == 7);
        }

        SECTION("Failure start") {
            CHECK(!ctx.idx.empty());
            CHECK(ctx.idx.back() == 0);
            CHECK(string_check.match(ctx, 'c') == fil::descpa::match_result::FAILURE);
            CHECK(!ctx.idx.empty());
            CHECK(ctx.idx.back() == 0);
        }

        SECTION("Failure then restart") {
            CHECK(string_check.match(ctx, 'c') == fil::descpa::match_result::FAILURE);
            CHECK(!ctx.idx.empty());
            CHECK(ctx.idx.back() == 0);
            CHECK(string_check.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(!ctx.idx.empty());
            CHECK(ctx.idx.back() == 1);
        }

        SECTION("Failure middle way") {
            CHECK(ctx.idx.back() == 0);
            CHECK(string_check.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.back() == 1);
            CHECK(string_check.match(ctx, 'H') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.back() == 2);
            CHECK(string_check.match(ctx, 'O') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.back() == 3);
            CHECK(string_check.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.back() == 4);
            CHECK(string_check.match(ctx, 'X') == fil::descpa::match_result::FAILURE);
            CHECK(ctx.idx.back() == 0);

            SECTION("success afterwards") {
                CHECK(string_check.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'H') == fil::descpa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'O') == fil::descpa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'O') == fil::descpa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'B') == fil::descpa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'O') == fil::descpa::match_result::SUCCESS);
                CHECK(ctx.idx.back() == 7);
            }
        }
    }

    SECTION("test composition") {
        SECTION("compose 3 char") {

            SECTION("direct instantiation") {
                fil::descpa::tuple_rule<fil::descpa::match_char<'L'>, fil::descpa::match_char<'O'>, fil::descpa::match_char<'L'>> composed;

                CHECK(ctx.idx.back() == 0);
                CHECK(composed.match(ctx, 'L') == fil::descpa::match_result::CONTINUE);
                CHECK(ctx.idx.back() == 1);
                CHECK(composed.match(ctx, 'O') == fil::descpa::match_result::CONTINUE);
                CHECK(ctx.idx.back() == 2);
                CHECK(composed.match(ctx, 'L') == fil::descpa::match_result::SUCCESS);
                CHECK(ctx.idx.back() == 3);
            }
            SECTION("+ instantiation") {
                constexpr auto composed =
                    fil::descpa::match_char<'L'> {} + fil::descpa::match_char<'O'> {} + fil::descpa::match_char<'L'> {};

                CHECK(ctx.idx.back() == 0);
                CHECK(composed.match(ctx, 'L') == fil::descpa::match_result::CONTINUE);
                CHECK(ctx.idx.back() == 1);
                CHECK(composed.match(ctx, 'O') == fil::descpa::match_result::CONTINUE);
                CHECK(ctx.idx.back() == 2);
                CHECK(composed.match(ctx, 'L') == fil::descpa::match_result::SUCCESS);
                CHECK(ctx.idx.back() == 3);
            }
        }
        SECTION("compose 3 string") {
            constexpr auto composed = fil::descpa::match_string<fil::descpa::fixed_string {"WE"}> {}
                                    + fil::descpa::match_string<fil::descpa::fixed_string {"Love"}> {}
                                    + fil::descpa::match_string<fil::descpa::fixed_string {"Chocobo"}> {};

            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 0);
            CHECK(composed.match(ctx, 'W') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx.back() == 1);
            CHECK(composed.match(ctx, 'E') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1); // decrease size as We is finished
            CHECK(ctx.idx.back() == 1);
            CHECK(composed.match(ctx, 'L') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2); // start Love
            CHECK(ctx.idx[0] == 1);     // still matching word index 1 (Love)
            CHECK(ctx.idx[1] == 1);     // after matching first char (index 0) 'L' we get to index 1 'o'
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx[0] == 1);     // still matching word index 1 (Love)
            CHECK(ctx.idx[1] == 2);     // to index 2 'v'
            CHECK(composed.match(ctx, 'v') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx[0] == 1);     // still matching word index 1 (Love)
            CHECK(ctx.idx[1] == 3);     // to index 2 'e'
            CHECK(composed.match(ctx, 'e') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1); // decreased as Love is finished
            CHECK(ctx.idx[0] == 2);     // Get to the index word 3 Chocobo
            CHECK(composed.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2); // started word Chocobo (increased)
            CHECK(ctx.idx[0] == 2);     // Get to the index word 3 Chocobo
            CHECK(ctx.idx[1] == 1);     // after index 0 'C' to index 1 'h'
            CHECK(composed.match(ctx, 'h') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx[0] == 2);     // Get to the index word 3 Chocobo
            CHECK(ctx.idx[1] == 2);     // to index 2 'o'
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx[0] == 2);     // Get to the index word 3 Chocobo
            CHECK(ctx.idx[1] == 3);     // to index 3 'c'
            CHECK(composed.match(ctx, 'c') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx[0] == 2);     // Get to the index word 3 Chocobo
            CHECK(ctx.idx[1] == 4);     // index 4 'o'
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx[0] == 2);     // Get to the index word 3 Chocobo
            CHECK(ctx.idx[1] == 5);     // to index 1 'b'
            CHECK(composed.match(ctx, 'b') == fil::descpa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx[0] == 2);     // Get to the index word 3 Chocobo
            CHECK(ctx.idx[1] == 6);     // to index 6 'o'
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::SUCCESS);
            CHECK(ctx.idx.size() == 1); // word Chocobo is finished -- finished the composed
            CHECK(ctx.idx[0] == 3);     // index above the limit of composed

            CHECK(composed.match(ctx, 'C') == fil::descpa::match_result::FAILURE); // an error occurs now, the composed is finished
        }
        SECTION("utility wrapped {}") {
            constexpr auto composed = fil::descpa::bracket_wrapped<                //
                fil::descpa::match_string<fil::descpa::fixed_string {"Chocobo"}>> {};

            CHECK(composed.match(ctx, '{') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'c') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'b') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, '}') == fil::descpa::match_result::SUCCESS);
        }

        SECTION("utility wrapped []") {
            constexpr auto composed = fil::descpa::square_wrapped< //
                fil::descpa::match_string<fil::descpa::fixed_string {"Chocobo"}>> {};

            CHECK(composed.match(ctx, '[') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'c') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'b') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, ']') == fil::descpa::match_result::SUCCESS);
        }

        SECTION("utility wrapped <>") {
            constexpr auto composed = fil::descpa::angle_wrapped< //
                fil::descpa::match_string<fil::descpa::fixed_string {"Chocobo"}>> {};

            CHECK(composed.match(ctx, '<') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'c') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'b') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, '>') == fil::descpa::match_result::SUCCESS);
        }
        SECTION("utility wrapped ()") {
            constexpr auto composed = fil::descpa::parenthesis_wrapped< //
                fil::descpa::match_string<fil::descpa::fixed_string {"Chocobo"}>> {};

            CHECK(composed.match(ctx, '(') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'c') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'b') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, ')') == fil::descpa::match_result::SUCCESS);
        }
        SECTION("utility ;") {
            constexpr auto composed = fil::descpa::match_string<fil::descpa::fixed_string {"Chocobo"}> {} + fil::descpa::match_semicol {};

            CHECK(composed.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'c') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'b') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, ';') == fil::descpa::match_result::SUCCESS);
        }
        SECTION("utility ,") {
            constexpr auto composed = fil::descpa::match_string<fil::descpa::fixed_string {"Chocobo"}> {} + fil::descpa::match_comma {};

            CHECK(composed.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'c') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'b') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, ',') == fil::descpa::match_result::SUCCESS);
        }
        SECTION("utility if") {
            constexpr auto composed = fil::descpa::match_if {}          //
                                    + fil::descpa::parenthesis_wrapped< //
                                          fil::descpa::match_string<fil::descpa::fixed_string {"Chocobo"}>> {};

            CHECK(composed.match(ctx, 'i') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'f') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, '(') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'c') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'b') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, ')') == fil::descpa::match_result::SUCCESS);
        }
        SECTION("utility while") {
            constexpr auto composed = fil::descpa::match_while {}       //
                                    + fil::descpa::parenthesis_wrapped< //
                                          fil::descpa::match_string<fil::descpa::fixed_string {"Chocobo"}>> {};

            CHECK(composed.match(ctx, 'w') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'i') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'l') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'e') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, '(') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'c') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'b') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::descpa::match_result::CONTINUE);
            CHECK(composed.match(ctx, ')') == fil::descpa::match_result::SUCCESS);
        }
    }
}

TEST_CASE("descpa file tests", "[descpa]") {

    fil::descpa::details_::matcher_ctx<fil::descpa::ast::aggregator<ast_object>> ctx;

    SECTION("test parse file single char") {
        const auto f1 = fil::temporary_file("I");
        fil::file_reader file_reader {f1};

        struct grammar {
            struct ast_object {
                char value;
            };

            static constexpr fil::descpa::rule auto rules() { return fil::descpa::match_char<'I'> {}; }
            static constexpr auto convertor() { return fil::descpa::ast::aggregator<ast_object> {}; }
        };

        auto g       = grammar {};
        const auto v = fil::descpa::parse(g, std::move(file_reader));
    }

    SECTION("test parse file string") {

        const auto f1 = fil::temporary_file("ILoveChocobo");
        fil::file_reader file_reader {f1};

        struct grammar {
            static constexpr fil::descpa::rule auto rules() {
                return fil::descpa::match_string<fil::descpa::fixed_string {"ILoveChocobo"}, fil::descpa::member<&ast_object::value>> {};
            }
            static constexpr auto convertor() { return fil::descpa::ast::aggregator<ast_object> {}; }
        };

        auto g       = grammar {};
        const auto v = fil::descpa::parse(g, std::move(file_reader));

        CHECK(v.value == "ILoveChocobo");
    }
}
