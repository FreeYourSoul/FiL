#include <catch2/catch_all.hpp>

#include "fil/copa/ast.hh"
#include "fil/copa/copa.hh"
#include "fil/copa/member.hh"
#include "fil/copa/sink.hh"
#include "fil/meta/buffer_reader.hh"

enum class op : int {
    INVALID,
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
    return op::INVALID;
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

struct calculator_grammar;
struct base_grammar {
    using ast_object = ast_node;

    static constexpr auto rules();
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

constexpr auto base_grammar::rules() {
    return fil::copa::match_number<ast_node::leaf> {}
         | fil::copa::parenthesised(fil::copa::match_parser<calculator_grammar, ast_node::leaf> {});
}

TEST_CASE("Copa :calculator parsing") {
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
    }
}