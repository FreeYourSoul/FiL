#include <catch2/catch_all.hpp>

#include "fil/copa/ast.hh"
#include "fil/copa/copa.hh"
#include "fil/copa/member.hh"
#include "fil/copa/sink.hh"
#include "fil/meta/buffer_reader.hh"

enum class op : int {
    none,
    plus,
    minus,
    multiply,
    divide
};
using ast_node = fil::copa::ast_node<[](const std::string& token) -> op {
    if (token == "+")
        return op::plus;
    if (token == "-")
        return op::minus;
    if (token == "*")
        return op::multiply;
    if (token == "/")
        return op::divide;
    return op::none;
}>;

struct level_2_grammar {
    using ast_object = ast_node;

    static constexpr auto rules() {
        return fil::copa::match_string<fil::fixed_string {"*"}, ast_node::operand> {} //
             | fil::copa::match_string<fil::fixed_string {"/"}, ast_node::operand> {};
    }
    static constexpr auto convertor() { return fil::copa::sink::ast_tree_generator<ast_node> {2}; }
};

struct level_1_grammar {
    using ast_object = ast_node;

    static constexpr auto rules() {
        return fil::copa::match_string<fil::fixed_string {"+"}, ast_node::operand> {} //
             | fil::copa::match_string<fil::fixed_string {"-"}, ast_node::operand> {};
    }
    static constexpr auto convertor() { return fil::copa::sink::ast_tree_generator<ast_node> {1}; }
};

struct base_grammar {
    using ast_object = ast_node;

    static constexpr fil::copa::rule auto rules();
    static constexpr auto convertor() { return fil::copa::sink::ast_tree_generator<ast_node> {0}; }
};

struct calculator_grammar {
    using ast_object = ast_node;

    static constexpr auto rules() {
        return fil::copa::list_rule<fil::copa::or_rule< //
            fil::copa::match_parser<base_grammar>,      //
            fil::copa::match_parser<level_1_grammar>,   //
            fil::copa::match_parser<level_2_grammar>    //
            >> {};
    }
    static constexpr auto convertor() { return fil::copa::sink::ast_tree_generator<ast_node> {0}; }
};

constexpr fil::copa::rule auto base_grammar::rules() {
    return fil::copa::match_number<ast_node::leaf> {}
         | fil::copa::parenthesised(fil::copa::match_production<calculator_grammar, ast_node::leaf> {});
}

TEST_CASE("Copa :calculator parsing", "[copa][calculator]") {
    SECTION("parse : parenthesis") {
        fil::buffer_reader reader("16 * (1337 + 42)");

        /* sub graph is the addition -- added as a leaf to the multiplication
                 *
               /   \
              16     +
                   /   \
                 1337   42
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::multiply);
        CHECK(std::get<int>(result.value().lhs) == 16);

        auto rhs = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs != nullptr);
        CHECK(rhs->value == op::plus);
        CHECK(std::get<int>(rhs->lhs) == 1337);
        CHECK(std::get<int>(rhs->rhs) == 42);
    }

    SECTION("parse : simple addition") {
        fil::buffer_reader reader("3 + 5");

        /*
                 +
               /   \
              3     5
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::plus);
        CHECK(std::holds_alternative<int>(result.value().lhs));
        CHECK(std::holds_alternative<int>(result.value().rhs));
        CHECK(std::get<int>(result.value().lhs) == 3);
        CHECK(std::get<int>(result.value().rhs) == 5);
    }

    SECTION("parse : simple subtraction") {
        fil::buffer_reader reader("10 - 7");

        /*
                 -
               /   \
              10    7
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::minus);
        CHECK(std::get<int>(result.value().lhs) == 10);
        CHECK(std::get<int>(result.value().rhs) == 7);
    }

    SECTION("parse : simple multiplication") {
        fil::buffer_reader reader("6 * 9");

        /*
                 *
               /   \
              6     9
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::multiply);
        CHECK(std::get<int>(result.value().lhs) == 6);
        CHECK(std::get<int>(result.value().rhs) == 9);
    }

    SECTION("parse : simple division") {
        fil::buffer_reader reader("20 / 4");

        /*
                 /
               /   \
              20    4
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::divide);
        CHECK(std::get<int>(result.value().lhs) == 20);
        CHECK(std::get<int>(result.value().rhs) == 4);
    }

    SECTION("parse : multiply before add (precedence)") {
        fil::buffer_reader reader("1 + 2 * 3");

        /*   + has lower precedence, * binds tighter on the right
                 +
               /   \
              1     *
                  /   \
                 2     3
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::plus);
        CHECK(std::get<int>(result.value().lhs) == 1);

        auto rhs = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs != nullptr);
        CHECK(rhs->value == op::multiply);
        CHECK(std::get<int>(rhs->lhs) == 2);
        CHECK(std::get<int>(rhs->rhs) == 3);
    }

    SECTION("parse : multiply then add (left to right precedence)") {
        fil::buffer_reader reader("2 * 3 + 4");

        /*   * has higher precedence so it becomes a subtree on the left
                 +
               /   \
              *     4
            /   \
           2     3
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::plus);
        CHECK(std::get<int>(result.value().rhs) == 4);

        auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs != nullptr);
        CHECK(lhs->value == op::multiply);
        CHECK(std::get<int>(lhs->lhs) == 2);
        CHECK(std::get<int>(lhs->rhs) == 3);
    }

    SECTION("parse : chained addition") {
        fil::buffer_reader reader("1 + 2 + 3");

        /*   same precedence, chains right
                 +
               /   \
              1     +
                  /   \
                 2     3
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::plus);
        CHECK(std::get<int>(result.value().lhs) == 1);

        auto rhs = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs != nullptr);
        CHECK(rhs->value == op::plus);
        CHECK(std::get<int>(rhs->lhs) == 2);
        CHECK(std::get<int>(rhs->rhs) == 3);
    }

    SECTION("parse : chained multiplication") {
        fil::buffer_reader reader("2 * 3 * 4");

        /*   same precedence, chains right
                 *
               /   \
              2     *
                  /   \
                 3     4
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::multiply);
        CHECK(std::get<int>(result.value().lhs) == 2);

        auto rhs = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs != nullptr);
        CHECK(rhs->value == op::multiply);
        CHECK(std::get<int>(rhs->lhs) == 3);
        CHECK(std::get<int>(rhs->rhs) == 4);
    }

    SECTION("parse : division before subtraction") {
        fil::buffer_reader reader("10 - 6 / 2");

        /*   / has higher precedence than -
                 -
               /   \
              10    /
                  /   \
                 6     2
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::minus);
        CHECK(std::get<int>(result.value().lhs) == 10);

        auto rhs = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs != nullptr);
        CHECK(rhs->value == op::divide);
        CHECK(std::get<int>(rhs->lhs) == 6);
        CHECK(std::get<int>(rhs->rhs) == 2);
    }

    SECTION("parse : division then subtraction") {
        fil::buffer_reader reader("6 / 2 - 1");

        /*   / has higher precedence so it becomes a subtree on the left
                 -
               /   \
              /     1
            /   \
           6     2
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::minus);
        CHECK(std::get<int>(result.value().rhs) == 1);

        auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs != nullptr);
        CHECK(lhs->value == op::divide);
        CHECK(std::get<int>(lhs->lhs) == 6);
        CHECK(std::get<int>(lhs->rhs) == 2);
    }

    SECTION("parse : mixed all four operators") {
        fil::buffer_reader reader("1 + 2 * 3 - 4 / 2");

        /*   precedence: * and / bind tighter than + and -
                         -
                       /   \
                      +     /
                    /   \  /  \
                   1    * 4    2
                      /   \
                     2     3
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::minus);
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().lhs));
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().rhs));

        // right branch: 4 / 2
        auto rhs_div = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs_div != nullptr);
        CHECK(rhs_div->value == op::divide);
        CHECK(std::get<int>(rhs_div->lhs) == 4);
        CHECK(std::get<int>(rhs_div->rhs) == 2);

        // left branch: 1 + (2 * 3)
        auto lhs_plus = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs_plus != nullptr);
        CHECK(lhs_plus->value == op::plus);
        CHECK(std::get<int>(lhs_plus->lhs) == 1);

        auto lhs_mul = std::get<std::shared_ptr<ast_node>>(lhs_plus->rhs);
        REQUIRE(lhs_mul != nullptr);
        CHECK(lhs_mul->value == op::multiply);
        CHECK(std::get<int>(lhs_mul->lhs) == 2);
        CHECK(std::get<int>(lhs_mul->rhs) == 3);
    }

    SECTION("parse : parenthesis override precedence (add before multiply)") {
        fil::buffer_reader reader("(1 + 2) * 3");

        /*   parentheses force addition first, then multiply
                 *
               /   \
              +     3
            /   \
           1     2
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::multiply);
        CHECK(std::get<int>(result.value().rhs) == 3);

        auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs != nullptr);
        CHECK(lhs->value == op::plus);
        CHECK(std::get<int>(lhs->lhs) == 1);
        CHECK(std::get<int>(lhs->rhs) == 2);
    }

    SECTION("parse : parenthesis simple") {
        fil::buffer_reader reader("(1 + 2)");

        /*   parentheses addition in parenthesis
                   (none)
                    /  \
                  +   (noset)
                /   \
               1     2
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::none);
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().lhs));
        CHECK(std::holds_alternative<std::monostate>(result.value().rhs));

        auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs != nullptr);
        CHECK(std::get<int>(lhs->lhs) == 1);
        CHECK(std::get<int>(lhs->rhs) == 2);
    }

    SECTION("parse : parenthesis on the left") {
        fil::buffer_reader reader("(1 + 2) * 3");

        /*   parentheses addition in parenthesis
                     *
                   /   \
                  +     3
                /   \
               1     2
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::multiply);
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().lhs));
        CHECK(std::get<int>(result.value().rhs) == 3);

        auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs != nullptr);
        CHECK(lhs->value == op::plus);
        CHECK(std::get<int>(lhs->lhs) == 1);
        CHECK(std::get<int>(lhs->rhs) == 2);
    }

    SECTION("parse : nested parentheses on left") {
        fil::buffer_reader reader("((1 + 2) + 3) + 4");

        /*   double nested parentheses around a simple addition
                        +
                      /   \
                     +     4
                  /    \
                 +      3
               /   \
              1     2
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::plus);
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().lhs));
        CHECK(std::get<int>(result.value().rhs) == 4);

        auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs != nullptr);
        CHECK(lhs->value == op::plus);
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(lhs->lhs));
        CHECK(std::get<int>(lhs->rhs) == 3);

        auto lhs2 = std::get<std::shared_ptr<ast_node>>(lhs->lhs);
        REQUIRE(lhs2 != nullptr);
        CHECK(lhs2->value == op::plus);
        CHECK(std::get<int>(lhs2->lhs) == 1);
        CHECK(std::get<int>(lhs2->rhs) == 2);
    }

    SECTION("parse : nested parentheses on right") {
        fil::buffer_reader reader("(1 + (2 + (3 + 4))");

        /*   double nested parentheses around a simple addition
                          (none)
                         /    \
                        +     (noset)
                      /   \
                     1     +
                         /   \
                        2     +
                            /   \
                           3     4
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::none);
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().lhs));
        CHECK(std::holds_alternative<std::monostate>(result.value().rhs));

        auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);

        CHECK(lhs->value == op::plus);
        CHECK(std::get<int>(lhs->lhs) == 1);
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(lhs->rhs));

        auto rhs = std::get<std::shared_ptr<ast_node>>(lhs->rhs);
        REQUIRE(rhs != nullptr);
        CHECK(rhs->value == op::plus);
        CHECK(std::get<int>(rhs->lhs) == 2);
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(rhs->rhs));

        auto rhs2 = std::get<std::shared_ptr<ast_node>>(rhs->rhs);
        REQUIRE(rhs2 != nullptr);
        CHECK(rhs2->value == op::plus);
        CHECK(std::get<int>(rhs2->lhs) == 3);
        CHECK(std::get<int>(rhs2->rhs) == 4);
    }

    SECTION("parse : nested parentheses") {
        fil::buffer_reader reader("((1 + 2))");

        /*   double nested parentheses around a simple addition
                 +
               /   \
              1     2
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::none);
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().lhs));
        CHECK(std::holds_alternative<std::monostate>(result.value().rhs));

        auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);

        CHECK(lhs->value == op::none);
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(lhs->lhs));
        CHECK(std::holds_alternative<std::monostate>(lhs->rhs));

        auto lhs2 = std::get<std::shared_ptr<ast_node>>(lhs->lhs);
        CHECK(std::holds_alternative<int>(lhs2->lhs));
        CHECK(std::holds_alternative<int>(lhs2->rhs));
        CHECK(std::get<int>(lhs2->lhs) == 1);
        CHECK(std::get<int>(lhs2->rhs) == 2);
    }

    SECTION("parse : parenthesised left and right") {
        fil::buffer_reader reader("(1 + 2) * (3 + 4)");

        /*   both sides parenthesised
                    *
                  /   \
                 +     +
               /  \  /   \
              1   2  3    4
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::multiply);

        auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs != nullptr);
        CHECK(lhs->value == op::plus);
        CHECK(std::get<int>(lhs->lhs) == 1);
        CHECK(std::get<int>(lhs->rhs) == 2);

        auto rhs = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs != nullptr);
        CHECK(rhs->value == op::plus);
        CHECK(std::get<int>(rhs->lhs) == 3);
        CHECK(std::get<int>(rhs->rhs) == 4);
    }

    SECTION("parse : deeply nested parentheses") {
        fil::buffer_reader reader("((2 + 3) * (4 - 1))");

        /*   outer parens wrap a multiplication of two parenthesised sub-expressions
                     (none)
                     /   \
                    *    (noset)
                  /   \
                 +     -
               /  \  /   \
              2   3  4    1
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::none);
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().lhs));
        CHECK(std::holds_alternative<std::monostate>(result.value().rhs));

        auto l = std::get<std::shared_ptr<ast_node>>(result.value().lhs);

        CHECK(l->value == op::multiply);
        auto lhs = std::get<std::shared_ptr<ast_node>>(l->lhs);
        REQUIRE(lhs != nullptr);
        CHECK(lhs->value == op::plus);
        CHECK(std::get<int>(lhs->lhs) == 2);
        CHECK(std::get<int>(lhs->rhs) == 3);

        auto rhs = std::get<std::shared_ptr<ast_node>>(l->rhs);
        REQUIRE(rhs != nullptr);
        CHECK(rhs->value == op::minus);
        CHECK(std::get<int>(rhs->lhs) == 4);
        CHECK(std::get<int>(rhs->rhs) == 1);
    }

    SECTION("parse : parenthesis on right side of subtraction") {
        fil::buffer_reader reader("100 - (50 + 25)");

        /*   parentheses group the addition on the right
                 -
               /   \
             100    +
                  /   \
                 50   25
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::minus);
        CHECK(std::get<int>(result.value().lhs) == 100);

        auto rhs = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs != nullptr);
        CHECK(rhs->value == op::plus);
        CHECK(std::get<int>(rhs->lhs) == 50);
        CHECK(std::get<int>(rhs->rhs) == 25);
    }

    SECTION("parse : parenthesis with division") {
        fil::buffer_reader reader("(10 - 2) / 4");

        /*   parenthesised subtraction divided by 4
                 /
               /   \
              -     4
            /   \
          10     2
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::divide);
        CHECK(std::get<int>(result.value().rhs) == 4);

        auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs != nullptr);
        CHECK(lhs->value == op::minus);
        CHECK(std::get<int>(lhs->lhs) == 10);
        CHECK(std::get<int>(lhs->rhs) == 2);
    }

    SECTION("parse : three-level precedence chain") {
        fil::buffer_reader reader("1 + 2 * 3 + 4");

        /*   multiplication binds tighter, then two additions chain
                        +
                      /   \
                     +     4
                   /   \
                  1     *
                      /   \
                     2     3
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::plus);
        CHECK(std::get<int>(result.value().rhs) == 4);

        auto lhs_plus = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs_plus != nullptr);
        CHECK(lhs_plus->value == op::plus);
        CHECK(std::get<int>(lhs_plus->lhs) == 1);

        auto mul_node = std::get<std::shared_ptr<ast_node>>(lhs_plus->rhs);
        REQUIRE(mul_node != nullptr);
        CHECK(mul_node->value == op::multiply);
        CHECK(std::get<int>(mul_node->lhs) == 2);
        CHECK(std::get<int>(mul_node->rhs) == 3);
    }

    SECTION("parse : complex expression with parens and mixed ops") {
        fil::buffer_reader reader("(1 + 2) * 3 - 4");

        /*   parenthesised add then multiply, then subtract
                    -
                  /   \
                 *     4
               /   \
              +     3
            /   \
           1     2
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::minus);
        CHECK(std::get<int>(result.value().rhs) == 4);

        auto lhs_mul = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs_mul != nullptr);
        CHECK(lhs_mul->value == op::multiply);
        CHECK(std::get<int>(lhs_mul->rhs) == 3);

        auto lhs_add = std::get<std::shared_ptr<ast_node>>(lhs_mul->lhs);
        REQUIRE(lhs_add != nullptr);
        CHECK(lhs_add->value == op::plus);
        CHECK(std::get<int>(lhs_add->lhs) == 1);
        CHECK(std::get<int>(lhs_add->rhs) == 2);
    }

    SECTION("parse : multiply and divide same precedence chain") {
        fil::buffer_reader reader("12 * 3 / 2");

        /*   * and / have same precedence, chains right
                 *
               /   \
              12    /
                  /   \
                 3     2
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::multiply);
        CHECK(std::get<int>(result.value().lhs) == 12);

        auto rhs = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs != nullptr);
        CHECK(rhs->value == op::divide);
        CHECK(std::get<int>(rhs->lhs) == 3);
        CHECK(std::get<int>(rhs->rhs) == 2);
    }

    SECTION("parse : add and subtract same precedence chain") {
        fil::buffer_reader reader("5 + 3 - 1");

        /*   + and - have same precedence, chains right
                 +
               /   \
              5     -
                  /   \
                 3     1
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::plus);
        CHECK(std::get<int>(result.value().lhs) == 5);

        auto rhs = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs != nullptr);
        CHECK(rhs->value == op::minus);
        CHECK(std::get<int>(rhs->lhs) == 3);
        CHECK(std::get<int>(rhs->rhs) == 1);
    }

    SECTION("parse : parenthesised single number in expression") {
        fil::buffer_reader reader("(5) + 3");

        /*   parenthesised 5 is just a leaf
                 +
               /   \
              5     3
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::plus);
        CHECK(std::get<int>(result.value().rhs) == 3);
    }

    SECTION("parse : parenthesised multiplication inside addition") {
        fil::buffer_reader reader("1 + (2 * 3) + 4");

        /*   parenthesised multiplication is a leaf within the addition chain
                     +
                   /   \
                  1     +
                      /   \
                     *     4
                   /   \
                  2     3
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::plus);
        CHECK(std::get<int>(result.value().lhs) == 1);

        auto rhs_plus = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs_plus != nullptr);
        CHECK(rhs_plus->value == op::plus);
        CHECK(std::get<int>(rhs_plus->rhs) == 4);

        auto mul_node = std::get<std::shared_ptr<ast_node>>(rhs_plus->lhs);
        REQUIRE(mul_node != nullptr);
        CHECK(mul_node->value == op::multiply);
        CHECK(std::get<int>(mul_node->lhs) == 2);
        CHECK(std::get<int>(mul_node->rhs) == 3);
    }

    SECTION("parse : long chain of additions") {
        fil::buffer_reader reader("1 + 2 + 3 + 4 + 5");

        /*   all same precedence, chains right
                 +
               /   \
              1     +
                  /   \
                 2     +
                     /   \
                    3     +
                        /   \
                       4     5
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::plus);
        CHECK(std::get<int>(result.value().lhs) == 1);

        auto n1 = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(n1 != nullptr);
        CHECK(n1->value == op::plus);
        CHECK(std::get<int>(n1->lhs) == 2);

        auto n2 = std::get<std::shared_ptr<ast_node>>(n1->rhs);
        REQUIRE(n2 != nullptr);
        CHECK(n2->value == op::plus);
        CHECK(std::get<int>(n2->lhs) == 3);

        auto n3 = std::get<std::shared_ptr<ast_node>>(n2->rhs);
        REQUIRE(n3 != nullptr);
        CHECK(n3->value == op::plus);
        CHECK(std::get<int>(n3->lhs) == 4);
        CHECK(std::get<int>(n3->rhs) == 5);
    }

    SECTION("parse : multiple parenthesised groups with multiply") {
        fil::buffer_reader reader("(1 + 2) * (3 - 4) * (5 + 6)");

        /*   three parenthesised groups chained by multiplication
                    *
                  /   \
                 +     *
               /  \  /   \
              1   2  -    +
                   /  \ /   \
                  3   4 5    6
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::multiply);

        auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs != nullptr);
        CHECK(lhs->value == op::plus);
        CHECK(std::get<int>(lhs->lhs) == 1);
        CHECK(std::get<int>(lhs->rhs) == 2);

        auto rhs_mul = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs_mul != nullptr);
        CHECK(rhs_mul->value == op::multiply);

        auto rhs_mul_lhs = std::get<std::shared_ptr<ast_node>>(rhs_mul->lhs);
        REQUIRE(rhs_mul_lhs != nullptr);
        CHECK(rhs_mul_lhs->value == op::minus);
        CHECK(std::get<int>(rhs_mul_lhs->lhs) == 3);
        CHECK(std::get<int>(rhs_mul_lhs->rhs) == 4);

        auto rhs_mul_rhs = std::get<std::shared_ptr<ast_node>>(rhs_mul->rhs);
        REQUIRE(rhs_mul_rhs != nullptr);
        CHECK(rhs_mul_rhs->value == op::plus);
        CHECK(std::get<int>(rhs_mul_rhs->lhs) == 5);
        CHECK(std::get<int>(rhs_mul_rhs->rhs) == 6);
    }

    SECTION("parse : parenthesis overriding right association") {
        fil::buffer_reader reader("(1 + 2) + 3");

        /*   parenthesised left side addition, then another addition
                    +
                  /   \
                 +     3
               /   \
              1     2
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::plus);
        CHECK(std::get<int>(result.value().rhs) == 3);

        auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs != nullptr);
        CHECK(lhs->value == op::plus);
        CHECK(std::get<int>(lhs->lhs) == 1);
        CHECK(std::get<int>(lhs->rhs) == 2);
    }

    SECTION("parse : complex with nested parens and all ops") {
        fil::buffer_reader reader("2 * (3 + 4) - (10 / 5)");

        /*   two parenthesised sub-expressions combined
                        -
                     /     \
                    *       /
                  /  \    /   \
                 2   +   10    5
                   /   \
                  3     4
        */

        calculator_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        fil::copa::debug::print_ast_tree(result.value());

        CHECK(result.value().value == op::minus);

        auto lhs_mul = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs_mul != nullptr);
        CHECK(lhs_mul->value == op::multiply);
        CHECK(std::get<int>(lhs_mul->lhs) == 2);

        auto lhs_add = std::get<std::shared_ptr<ast_node>>(lhs_mul->rhs);
        REQUIRE(lhs_add != nullptr);
        CHECK(lhs_add->value == op::plus);
        CHECK(std::get<int>(lhs_add->lhs) == 3);
        CHECK(std::get<int>(lhs_add->rhs) == 4);

        auto rhs_div = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs_div != nullptr);
        CHECK(rhs_div->value == op::divide);
        CHECK(std::get<int>(rhs_div->lhs) == 10);
        CHECK(std::get<int>(rhs_div->rhs) == 5);
    }
}