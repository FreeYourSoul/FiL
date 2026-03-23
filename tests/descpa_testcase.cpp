#include "descpa.hh"

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "fil/descpa/descpa.hh"

TEST_CASE("descpa basic tests", "[descpa]") {

    fil::descpa::details_::matcher_ctx ctx;
    ctx.idx.push_back(0);
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
    }
}