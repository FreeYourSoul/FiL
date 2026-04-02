#include "../include/fil/meta/buffer_reader.hh"
#include "ast.hh"

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

#include "fil/file/file_reader.hh"
#include "fil/file/temporary.hh"

#include "fil/copa/ast.hh"
#include "fil/copa/copa.hh"
#include "fil/copa/matcher.hh"
#include "fil/copa/sink.hh"

//@todo :: check that or EOF is working
//@todo :: check that EOF is working in general

TEST_CASE("Copa: matcher tests", "[copa]") {
    SECTION("test char match") {
        fil::copa::details_::rule_ctx<fil::copa::details_::reader_noop, fil::copa::sink::convertor_noop<char>> ctx;

        fil::copa::match_char<'X'> char_x_check;

        CHECK(char_x_check.match(ctx, 'X') == fil::copa::match_result::SUCCESS);
        CHECK(char_x_check.match(ctx, 'Y') == fil::copa::match_result::FAILURE);
        CHECK(char_x_check.match(ctx, 'x') == fil::copa::match_result::FAILURE);
    }
    SECTION("test string match") {
        fil::copa::details_::rule_ctx<fil::copa::details_::reader_noop, fil::copa::sink::convertor_noop<std::string>> ctx;

        fil::copa::match_string<fil::fixed_string {"CHOCOBO"}> string_check;

        SECTION("Success") {
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 0);
            CHECK(string_check.match(ctx, 'C') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 1);
            CHECK(string_check.match(ctx, 'H') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 2);
            CHECK(string_check.match(ctx, 'O') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 3);
            CHECK(string_check.match(ctx, 'C') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 4);
            CHECK(string_check.match(ctx, 'O') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 5);
            CHECK(string_check.match(ctx, 'B') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 6);
            CHECK(string_check.match(ctx, 'O') == fil::copa::match_result::SUCCESS);
            CHECK(ctx.idx.back() == 7);
        }

        SECTION("Failure start") {
            CHECK(!ctx.idx.empty());
            CHECK(ctx.idx.back() == 0);
            CHECK(string_check.match(ctx, 'c') == fil::copa::match_result::FAILURE);
            CHECK(!ctx.idx.empty());
            CHECK(ctx.idx.back() == 0);
        }

        SECTION("Failure then restart") {
            CHECK(string_check.match(ctx, 'c') == fil::copa::match_result::FAILURE);
            CHECK(!ctx.idx.empty());
            CHECK(ctx.idx.back() == 0);
            CHECK(string_check.match(ctx, 'C') == fil::copa::match_result::CONTINUE);
            CHECK(!ctx.idx.empty());
            CHECK(ctx.idx.back() == 1);
        }

        SECTION("Failure middle way") {
            CHECK(ctx.idx.back() == 0);
            CHECK(string_check.match(ctx, 'C') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.back() == 1);
            CHECK(string_check.match(ctx, 'H') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.back() == 2);
            CHECK(string_check.match(ctx, 'O') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.back() == 3);
            CHECK(string_check.match(ctx, 'C') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.back() == 4);
            CHECK(string_check.match(ctx, 'X') == fil::copa::match_result::FAILURE);
            CHECK(ctx.idx.back() == 0);

            SECTION("success afterwards") {
                CHECK(string_check.match(ctx, 'C') == fil::copa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'H') == fil::copa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'O') == fil::copa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'C') == fil::copa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'O') == fil::copa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'B') == fil::copa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'O') == fil::copa::match_result::SUCCESS);
                CHECK(ctx.idx.back() == 7);
            }
        }
    }
    SECTION("test composition") {
        SECTION("compose 3 char") {
            fil::copa::details_::rule_ctx<fil::copa::details_::reader_noop, fil::copa::sink::convertor_noop<char>> ctx;

            SECTION("direct instantiation") {
                fil::copa::tuple_rule<fil::copa::match_char<'L'>, fil::copa::match_char<'O'>, fil::copa::match_char<'L'>> composed;

                CHECK(ctx.idx.back() == 0);
                CHECK(composed.match(ctx, 'L') == fil::copa::match_result::CONTINUE);
                CHECK(ctx.idx.back() == 1);
                CHECK(composed.match(ctx, 'O') == fil::copa::match_result::CONTINUE);
                CHECK(ctx.idx.back() == 2);
                CHECK(composed.match(ctx, 'L') == fil::copa::match_result::SUCCESS);
                CHECK(ctx.idx.back() == 3);
            }
            SECTION("+ instantiation") {
                constexpr auto composed = fil::copa::match_char<'L'> {} + fil::copa::match_char<'O'> {} + fil::copa::match_char<'L'> {};

                CHECK(ctx.idx.back() == 0);
                CHECK(composed.match(ctx, 'L') == fil::copa::match_result::CONTINUE);
                CHECK(ctx.idx.back() == 1);
                CHECK(composed.match(ctx, 'O') == fil::copa::match_result::CONTINUE);
                CHECK(ctx.idx.back() == 2);
                CHECK(composed.match(ctx, 'L') == fil::copa::match_result::SUCCESS);
                CHECK(ctx.idx.back() == 3);
            }
        }
        SECTION("mixup string and char matching") {
            fil::copa::details_::rule_ctx<fil::copa::details_::reader_noop, fil::copa::sink::convertor_noop<>> ctx;
            fil::copa::tuple_rule<                                 //
                fil::copa::match_char<'L'>,                        //
                fil::copa::match_semicol,                          //
                fil::copa::match_string<fil::fixed_string {"dad"}> //
                >
                composed;

            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 0);
            CHECK(composed.match(ctx, 'L') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 1);
            CHECK(composed.match(ctx, ';') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 2);
            CHECK(composed.match(ctx, 'd') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx.back() == 1);
            CHECK(composed.match(ctx, 'a') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx.back() == 2);
            CHECK(composed.match(ctx, 'd') == fil::copa::match_result::SUCCESS);
            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 3);
        }

        SECTION("space_like match : all check") {
            fil::copa::details_::rule_ctx<fil::copa::details_::reader_noop, fil::copa::sink::convertor_noop<char>> ctx;
            fil::copa::tuple_rule<           //
                fil::copa::match_char<'L'>,  //
                fil::copa::match_space_like, //
                fil::copa::match_char<'O'>,  //
                fil::copa::match_space_like, //
                fil::copa::match_char<'L'>,  //
                fil::copa::match_space_like, //
                fil::copa::match_space_like, //
                fil::copa::match_space_like, //
                fil::copa::match_space_like  //
                >
                composed;

            CHECK(ctx.idx.back() == 0);
            CHECK(composed.match(ctx, 'L') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.back() == 1);
            CHECK(composed.match(ctx, ' ') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.back() == 2);
            CHECK(composed.match(ctx, 'O') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.back() == 3);
            CHECK(composed.match(ctx, '\t') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.back() == 4);
            CHECK(composed.match(ctx, 'L') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.back() == 5);
            CHECK(composed.match(ctx, '\r') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.back() == 6);
            CHECK(composed.match(ctx, '\f') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.back() == 7);
            CHECK(composed.match(ctx, '\n') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.back() == 8);
            CHECK(composed.match(ctx, '\v') == fil::copa::match_result::SUCCESS);
            CHECK(ctx.idx.back() == 9);
        }

        SECTION("compose 3 string") {
            fil::copa::details_::rule_ctx<fil::copa::details_::reader_noop, fil::copa::sink::convertor_noop<std::string>> ctx;
            constexpr auto composed = fil::copa::match_string<fil::fixed_string {"WE"}> {}
                                    + fil::copa::match_string<fil::fixed_string {"Love"}> {}
                                    + fil::copa::match_string<fil::fixed_string {"Chocobo"}> {};

            CHECK(ctx.idx.size() == 1);
            CHECK(ctx.idx.back() == 0);
            CHECK(composed.match(ctx, 'W') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx.back() == 1);
            CHECK(composed.match(ctx, 'E') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1); // decrease size as We is finished
            CHECK(ctx.idx.back() == 1);
            CHECK(composed.match(ctx, 'L') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2); // start Love
            CHECK(ctx.idx[0] == 1);     // still matching word index 1 (Love)
            CHECK(ctx.idx[1] == 1);     // after matching first char (index 0) 'L' we get to index 1 'o'
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx[0] == 1);     // still matching word index 1 (Love)
            CHECK(ctx.idx[1] == 2);     // to index 2 'v'
            CHECK(composed.match(ctx, 'v') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx[0] == 1);     // still matching word index 1 (Love)
            CHECK(ctx.idx[1] == 3);     // to index 2 'e'
            CHECK(composed.match(ctx, 'e') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 1); // decreased as Love is finished
            CHECK(ctx.idx[0] == 2);     // Get to the index word 3 Chocobo
            CHECK(composed.match(ctx, 'C') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2); // started word Chocobo (increased)
            CHECK(ctx.idx[0] == 2);     // Get to the index word 3 Chocobo
            CHECK(ctx.idx[1] == 1);     // after index 0 'C' to index 1 'h'
            CHECK(composed.match(ctx, 'h') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx[0] == 2);     // Get to the index word 3 Chocobo
            CHECK(ctx.idx[1] == 2);     // to index 2 'o'
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx[0] == 2);     // Get to the index word 3 Chocobo
            CHECK(ctx.idx[1] == 3);     // to index 3 'c'
            CHECK(composed.match(ctx, 'c') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx[0] == 2);     // Get to the index word 3 Chocobo
            CHECK(ctx.idx[1] == 4);     // index 4 'o'
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx[0] == 2);     // Get to the index word 3 Chocobo
            CHECK(ctx.idx[1] == 5);     // to index 1 'b'
            CHECK(composed.match(ctx, 'b') == fil::copa::match_result::CONTINUE);
            CHECK(ctx.idx.size() == 2);
            CHECK(ctx.idx[0] == 2);     // Get to the index word 3 Chocobo
            CHECK(ctx.idx[1] == 6);     // to index 6 'o'
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::SUCCESS);
            CHECK(ctx.idx.size() == 1); // word Chocobo is finished -- finished the composed
            CHECK(ctx.idx[0] == 3);     // index above the limit of composed

            CHECK(composed.match(ctx, 'C') == fil::copa::match_result::FAILURE); // an error occurs now, the composed is finished
        }
        SECTION("utility wrapped {}") {
            fil::copa::details_::rule_ctx<fil::copa::details_::reader_noop, fil::copa::sink::convertor_noop<std::string>> ctx;
            constexpr auto composed = fil::copa::bracket_wrapped< //
                fil::copa::match_string<fil::fixed_string {"Chocobo"}>> {};

            CHECK(composed.match(ctx, '{') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'C') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'c') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'b') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, '}') == fil::copa::match_result::SUCCESS);
        }

        SECTION("utility wrapped []") {
            fil::copa::details_::rule_ctx<fil::copa::details_::reader_noop, fil::copa::sink::convertor_noop<std::string>> ctx;
            constexpr auto composed = fil::copa::square_wrapped< //
                fil::copa::match_string<fil::fixed_string {"Chocobo"}>> {};

            CHECK(composed.match(ctx, '[') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'C') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'c') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'b') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, ']') == fil::copa::match_result::SUCCESS);
        }

        SECTION("utility wrapped <>") {
            fil::copa::details_::rule_ctx<fil::copa::details_::reader_noop, fil::copa::sink::convertor_noop<std::string>> ctx;
            constexpr auto composed = fil::copa::angle_wrapped< //
                fil::copa::match_string<fil::fixed_string {"Chocobo"}>> {};

            CHECK(composed.match(ctx, '<') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'C') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'c') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'b') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, '>') == fil::copa::match_result::SUCCESS);
        }
        SECTION("utility wrapped ()") {
            fil::copa::details_::rule_ctx<fil::copa::details_::reader_noop, fil::copa::sink::convertor_noop<std::string>> ctx;
            constexpr auto composed = fil::copa::parenthesis_wrapped< //
                fil::copa::match_string<fil::fixed_string {"Chocobo"}>> {};

            CHECK(composed.match(ctx, '(') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'C') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'c') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'b') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, ')') == fil::copa::match_result::SUCCESS);
        }
        SECTION("utility ;") {
            fil::copa::details_::rule_ctx<fil::copa::details_::reader_noop, fil::copa::sink::convertor_noop<std::string>> ctx;
            constexpr auto composed = fil::copa::match_string<fil::fixed_string {"Chocobo"}> {} + fil::copa::match_semicol {};

            CHECK(composed.match(ctx, 'C') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'c') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'b') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, ';') == fil::copa::match_result::SUCCESS);
        }
        SECTION("utility ,") {
            fil::copa::details_::rule_ctx<fil::copa::details_::reader_noop, fil::copa::sink::convertor_noop<std::string>> ctx;
            constexpr auto composed = fil::copa::match_string<fil::fixed_string {"Chocobo"}> {} + fil::copa::match_comma {};

            CHECK(composed.match(ctx, 'C') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'c') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'b') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, ',') == fil::copa::match_result::SUCCESS);
        }
        SECTION("utility if") {
            fil::copa::details_::rule_ctx<fil::copa::details_::reader_noop, fil::copa::sink::convertor_noop<std::string>> ctx;
            constexpr auto composed = fil::copa::match_if {}          //
                                    + fil::copa::parenthesis_wrapped< //
                                          fil::copa::match_string<fil::fixed_string {"Chocobo"}>> {};

            CHECK(composed.match(ctx, 'i') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'f') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, '(') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'C') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'c') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'b') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, ')') == fil::copa::match_result::SUCCESS);
        }
        SECTION("utility while") {
            fil::copa::details_::rule_ctx<fil::copa::details_::reader_noop, fil::copa::sink::convertor_noop<std::string>> ctx;
            constexpr auto composed = fil::copa::match_while {}       //
                                    + fil::copa::parenthesis_wrapped< //
                                          fil::copa::match_string<fil::fixed_string {"Chocobo"}>> {};

            CHECK(composed.match(ctx, 'w') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'i') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'l') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'e') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, '(') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'C') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'h') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'c') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'b') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, 'o') == fil::copa::match_result::CONTINUE);
            CHECK(composed.match(ctx, ')') == fil::copa::match_result::SUCCESS);
        }
    }
}

TEST_CASE("Copa: reader tests", "[copa][reader]") {

    SECTION("test parse file single char") {
        const auto f1 = fil::temporary_file("I");
        fil::file_reader file_reader {f1};

        struct grammar {
            struct ast_object {
                char value;
            };

            static constexpr fil::copa::rule auto rules() { return fil::copa::match_char<'I'> {}; }
            static constexpr auto convertor() { return fil::copa::sink::convertor_noop<ast_object> {}; }
        };

        auto g       = grammar {};
        const auto v = fil::copa::parse(g, std::move(file_reader));
    }

    SECTION("test identifier match") {
        const auto f = fil::temporary_file("chocobo ");
        fil::file_reader file_reader {f};

        struct grammar {
            struct ast_object {
                std::string value;
            };

            static constexpr fil::copa::rule auto rules() { return fil::copa::match_identifier<fil::copa::member<&ast_object::value>> {}; }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        auto g       = grammar {};
        const auto v = fil::copa::parse(g, std::move(file_reader));
        REQUIRE(v.has_value());
        CHECK(v.value().value == "chocobo");
    }

    SECTION("test parse file string") {
        const auto f1 = fil::temporary_file("ILoveChocobo");
        fil::file_reader file_reader {f1};

        struct grammar {
            struct ast_object {
                std::string value;
            };

            static constexpr fil::copa::rule auto rules() {
                return fil::copa::match_string<fil::fixed_string {"ILoveChocobo"}, fil::copa::member<&ast_object::value>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        auto g       = grammar {};
        const auto v = fil::copa::parse(g, std::move(file_reader));

        REQUIRE(v.has_value());
        CHECK(v.value().value == "ILoveChocobo");
    }
}

TEST_CASE("Copa: rule tests", "[copa]") {
    SECTION("multiple identifier") {
        fil::buffer_reader reader("chocobo is the best of the world ");

        struct grammar {
            struct ast_object {
                std::string word1;
                std::string word2;
                std::string word3;
                std::string word4;
                std::string word5;
                std::string word6;
                std::string word7;
            };

            static constexpr fil::copa::rule auto rules() {
                return                                                                      //
                    fil::copa::match_identifier<fil::copa::member<&ast_object::word1>> {} + //
                    fil::copa::match_identifier<fil::copa::member<&ast_object::word2>> {} + //
                    fil::copa::match_identifier<fil::copa::member<&ast_object::word3>> {} + //
                    fil::copa::match_identifier<fil::copa::member<&ast_object::word4>> {} + //
                    fil::copa::match_identifier<fil::copa::member<&ast_object::word5>> {} + //
                    fil::copa::match_identifier<fil::copa::member<&ast_object::word6>> {} + //
                    fil::copa::match_identifier<fil::copa::member<&ast_object::word7>> {};
            }

            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        auto g       = grammar {};
        const auto v = fil::copa::parse(g, std::move(reader));

        REQUIRE(v.has_value());
        CHECK(v.value().word1 == "chocobo");
        CHECK(v.value().word2 == "is");
        CHECK(v.value().word3 == "the");
        CHECK(v.value().word4 == "best");
        CHECK(v.value().word5 == "of");
        CHECK(v.value().word6 == "the");
        CHECK(v.value().word7 == "world");
    }
    SECTION("structure in structure") {
        fil::buffer_reader reader("chocobo is best ");

        struct inner_ast_object {
            std::string inner1;
            std::string inner2;
        };

        struct parser_custom : fil::copa::composable_rule {

            using ast_object = inner_ast_object;

            static constexpr fil::copa::rule auto rules() {
                return                                                                             //
                    fil::copa::match_identifier<fil::copa::member<&inner_ast_object::inner1>> {} + //
                    fil::copa::match_identifier<fil::copa::member<&inner_ast_object::inner2>> {};
            }

            static constexpr auto convertor() { return fil::copa::sink::aggregator<inner_ast_object> {}; }
        };

        struct grammar {

            struct ast_object {
                std::string value1;
                inner_ast_object value2;
            };

            static constexpr fil::copa::rule auto rules() {
                return                                                                       //
                    fil::copa::match_identifier<fil::copa::member<&ast_object::value1>> {} + //
                    fil::copa::match_parser<parser_custom, fil::copa::member<&ast_object::value2>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        auto g       = grammar {};
        const auto v = fil::copa::parse(g, std::move(reader));

        REQUIRE(v.has_value());
        CHECK(v.value().value1 == "chocobo");
        CHECK(v.value().value2.inner1 == "is");
        CHECK(v.value().value2.inner2 == "best");
    }
    SECTION("tuple check string in vector") {
        fil::buffer_reader reader("chocobo OfDoom ");

        struct grammar {

            struct ast_object {
                std::vector<std::string> vec_value {};
            };

            static constexpr fil::copa::rule auto rules() {
                return fil::copa::match_string<fil::fixed_string {"chocobo"}, fil::copa::member<&ast_object::vec_value>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        auto g       = grammar {};
        const auto v = fil::copa::parse(g, std::move(reader));

        REQUIRE(v.value().vec_value.size() == 1);
        CHECK(v.value().vec_value[0] == "chocobo");
    }
    SECTION("list simple") {
        fil::buffer_reader reader("chocobo is best ");

        struct grammar {

            struct ast_object {
                std::vector<std::string> vec_value {};
            };

            static constexpr fil::copa::rule auto rules() {
                return                                                                         //
                    fil::copa::list_rule<                                                      //
                        fil::copa::match_identifier<fil::copa::member<&ast_object::vec_value>> //
                        > {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        auto g       = grammar {};
        const auto v = fil::copa::parse(g, std::move(reader));

        REQUIRE(v.has_value());
        REQUIRE(v.value().vec_value.size() == 3);
        CHECK(v.value().vec_value[0] == "chocobo");
        CHECK(v.value().vec_value[1] == "is");
        CHECK(v.value().vec_value[2] == "best");
    }
    SECTION("repeat") {
        fil::buffer_reader reader("chocobo is best ");

        SECTION("in ast vector") {
            struct grammar {

                struct ast_object {
                    std::vector<std::string> vec_value {};
                };

                static constexpr fil::copa::rule auto rules() {
                    return fil::copa::repeat<3>(fil::copa::match_identifier<fil::copa::member<&ast_object::vec_value>> {});
                }
                static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
            };

            auto g       = grammar {};
            const auto v = fil::copa::parse(g, std::move(reader));

            REQUIRE(v.has_value());
            REQUIRE(v.value().vec_value.size() == 3);
            CHECK(v.value().vec_value[0] == "chocobo");
            CHECK(v.value().vec_value[1] == "is");
            CHECK(v.value().vec_value[2] == "best");
        }
    }
    SECTION("or_rule") {
        fil::buffer_reader reader("chocobo is best ");

        SECTION("or character : 2 options") {}
        SECTION("or string : 2 options") {
            struct grammar {
                struct ast_object {
                    std::string value {};
                };

                static constexpr fil::copa::rule auto rules() {
                    return fil::copa::or_rule<                                                                         //
                        fil::copa::match_string<fil::fixed_string {"chocobo"}, fil::copa::member<&ast_object::value>>, //
                        fil::copa::match_identifier<fil::copa::member<&ast_object::value>>                             //
                        > {};
                }
                static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
            };

            auto g       = grammar {};
            const auto v = fil::copa::parse(g, std::move(reader));

            REQUIRE(v.has_value());
            REQUIRE(v.value().value == "chocobo");
        }
        SECTION("or identifier : 2 options") {
            struct grammar {
                struct ast_object {
                    std::string value {};
                };

                static constexpr fil::copa::rule auto rules() {
                    return fil::copa::or_rule<                                                                       //
                        fil::copa::match_string<fil::fixed_string {"ifrit"}, fil::copa::member<&ast_object::value>>, //
                        fil::copa::match_identifier<fil::copa::member<&ast_object::value>>                           //
                        > {};
                }
                static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
            };

            auto g       = grammar {};
            const auto v = fil::copa::parse(g, std::move(reader));

            REQUIRE(v.has_value());
            REQUIRE(v.value().value == "chocobo");
        }

        SECTION("first match is taken") {
            SECTION("2 options") {}
            SECTION("5 options") {}
        }
    }
    SECTION("may_rule") {
        struct grammar {
            struct ast_object {
                std::string value {};
            };

            static constexpr fil::copa::rule auto rules() {
                return fil::copa::may_rule<fil::copa::match_identifier<fil::copa::member<&ast_object::value>>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        SECTION("match") {
            fil::buffer_reader reader("chocobo ");

            auto g       = grammar {};
            const auto v = fil::copa::parse(g, std::move(reader));

            REQUIRE(v.has_value());
            REQUIRE(v.value().value == "chocobo");
        }
        SECTION("not match") {
            fil::buffer_reader reader("");

            auto g       = grammar {};
            const auto v = fil::copa::parse(g, std::move(reader));

            REQUIRE(v.has_value());
            REQUIRE(v.value().value.empty());
        }

        SECTION("not match in composition") {
            struct grammar_may_composed {
                struct ast_object {
                    std::string value1 {};
                    std::string value2 {};
                };

                static constexpr fil::copa::rule auto rules() {
                    return fil::copa::may_rule<
                               fil::copa::match_string<fil::fixed_string {"NotMatched"}, fil::copa::member<&ast_object::value1>>> {} //
                         + fil::copa::match_identifier<fil::copa::member<&ast_object::value2>> {};
                }
                static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
            };

            fil::buffer_reader reader("chocobo ");

            auto g       = grammar_may_composed {};
            const auto v = fil::copa::parse(g, std::move(reader));

            REQUIRE(v.has_value());
            REQUIRE(v.value().value1.empty());
            REQUIRE(v.value().value2 == "chocobo");
        }
    }
}
