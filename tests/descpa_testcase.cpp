#include "descpa.hh"

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "fil/descpa/descpa.hh"

TEST_CASE("descpa basic tests", "[descpa]") {

    SECTION("test char") {

        fil::descpa::details_::matcher_ctx ctx;
        fil::descpa::match_char<'X'> char_x_check;

        CHECK(char_x_check.match(ctx, 'X') == fil::descpa::match_result::SUCCESS);
        CHECK(char_x_check.match(ctx, 'Y') == fil::descpa::match_result::FAILURE);
        CHECK(char_x_check.match(ctx, 'x') == fil::descpa::match_result::FAILURE);
    }
    SECTION("test string") {

        fil::descpa::details_::matcher_ctx ctx;
        fil::descpa::match_string<fil::descpa::fixed_string {"CHOCOBO"}> string_check;

        SECTION("Success") {
            CHECK(string_check.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(string_check.match(ctx, 'H') == fil::descpa::match_result::CONTINUE);
            CHECK(string_check.match(ctx, 'O') == fil::descpa::match_result::CONTINUE);
            CHECK(string_check.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(string_check.match(ctx, 'O') == fil::descpa::match_result::CONTINUE);
            CHECK(string_check.match(ctx, 'B') == fil::descpa::match_result::CONTINUE);
            CHECK(string_check.match(ctx, 'O') == fil::descpa::match_result::SUCCESS);
        }
        SECTION("Failure start") { CHECK(string_check.match(ctx, 'c') == fil::descpa::match_result::FAILURE); }
        SECTION("Failure then restart") {
            CHECK(string_check.match(ctx, 'c') == fil::descpa::match_result::FAILURE);
            CHECK(string_check.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
        }
        SECTION("Failure middle way") {
            CHECK(string_check.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(string_check.match(ctx, 'H') == fil::descpa::match_result::CONTINUE);
            CHECK(string_check.match(ctx, 'O') == fil::descpa::match_result::CONTINUE);
            CHECK(string_check.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
            CHECK(string_check.match(ctx, 'X') == fil::descpa::match_result::FAILURE);
            CHECK(string_check.match(ctx, 'O') == fil::descpa::match_result::FAILURE); // failure -- must restart from a C

            SECTION("success afterwards") {
                CHECK(string_check.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'H') == fil::descpa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'O') == fil::descpa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'C') == fil::descpa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'O') == fil::descpa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'B') == fil::descpa::match_result::CONTINUE);
                CHECK(string_check.match(ctx, 'O') == fil::descpa::match_result::SUCCESS);
            }
        }
    }
    SECTION("test composition") {

        fil::descpa::details_::matcher_ctx ctx;

        SECTION("compose 3 char") {

            SECTION("direct instantiation") {
                fil::descpa::tuple_rule<fil::descpa::match_char<'L'>, fil::descpa::match_char<'O'>, fil::descpa::match_char<'L'>> composed;

                CHECK(composed.match(ctx, 'L') == fil::descpa::match_result::CONTINUE);
            }
            SECTION("+ instantiation") {}
        }
    }
}