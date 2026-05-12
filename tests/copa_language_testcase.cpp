#include <catch2/catch_all.hpp>

#include "fil/copa/copa.hh"
#include "fil/copa/member.hh"
#include "fil/copa/sink.hh"
#include "fil/copa/wrapper_utils.hh"
#include "fil/meta/buffer_reader.hh"
#include "fil/meta/visit_utils.hh"

namespace {
enum class op_comparator {
    none,
    greater,
    greater_equal,
    less,
    less_equal,
    equal,
    different,
};
enum class op_link {
    none,
    or_,
    and_,
};

struct variable {
    std::string variable_name;
    std::optional<std::string> access = std::nullopt;

    [[maybe_unused]] [[nodiscard]] std::string to_string() const {
        return std::format("variable [name: {}, access: {}]", variable_name, access.value_or("nullopt"));
    }
};

struct variable_grammar {
    using ast_object = variable;

    static constexpr auto rules() {
        return (fil::copa::match_identifier<fil::copa::member<&ast_object::variable_name>> {} //
                + fil::copa::point                                                            //
                + fil::copa::match_identifier<fil::copa::member<&ast_object::access>> {})
             | fil::copa::match_identifier<fil::copa::member<&ast_object::variable_name>> {};
    }
    static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
};

using ast_node = fil::copa::ast_node< //
    [](const std::string& token) -> std::variant<op_link, op_comparator> {
        if (token == "&&")
            return op_link::and_;
        if (token == "||")
            return op_link::or_;

        if (token == ">")
            return op_comparator::greater;
        if (token == ">=")
            return op_comparator::greater_equal;
        if (token == "<=")
            return op_comparator::less_equal;
        if (token == "<")
            return op_comparator::less;
        if (token == "!=")
            return op_comparator::different;
        if (token == "==")
            return op_comparator::equal;

        return op_link::none;
    },
    variable>;

struct link_grammar {
    using ast_object = ast_node;

    static constexpr auto rules() {
        return fil::copa::match_string<fil::fixed_string {"&&"}, ast_object::operand> {} //
             | fil::copa::match_string<fil::fixed_string {"||"}, ast_object::operand> {};
    }
    static constexpr auto convertor() { return fil::copa::sink::ast_tree_generator<ast_object> {1}; }
};

struct compare_grammar {
    using ast_object = ast_node;

    static constexpr auto rules() {
        return fil::copa::match_string<fil::fixed_string {">="}, ast_object::operand> {} //
             | fil::copa::match_string<fil::fixed_string {">"}, ast_object::operand> {}
             | fil::copa::match_string<fil::fixed_string {"<="}, ast_object::operand> {}
             | fil::copa::match_string<fil::fixed_string {"<"}, ast_object::operand> {}
             | fil::copa::match_string<fil::fixed_string {"!="}, ast_object::operand> {}
             | fil::copa::match_string<fil::fixed_string {"=="}, ast_object::operand> {};
    }
    static constexpr auto convertor() { return fil::copa::sink::ast_tree_generator<ast_object> {2}; }
};

struct base_language_grammar {
    using ast_object = ast_node;

    static constexpr fil::copa::rule auto rules();
    static constexpr auto convertor() { return fil::copa::sink::ast_tree_generator<ast_object> {0}; }
};

struct language_grammar {
    using ast_object = ast_node;

    static constexpr auto rules() {
        return fil::copa::list_rule<fil::copa::or_rule<     //
            fil::copa::match_parser<base_language_grammar>, //
            fil::copa::match_parser<compare_grammar>,       //
            fil::copa::match_parser<link_grammar>           //
            >> {};
    }
    static constexpr auto convertor() { return fil::copa::sink::ast_tree_generator<ast_node> {0}; }
};
constexpr fil::copa::rule auto base_language_grammar::rules() {
    return fil::copa::match_number<ast_node::leaf> {}                       //
         | fil::copa::match_production<variable_grammar, ast_node::leaf> {} //
         | fil::copa::parenthesised(fil::copa::match_production<language_grammar, ast_node::leaf> {});
}
} // namespace

namespace fil {
template<>
[[nodiscard]] std::string to_string(const std::variant<op_link, op_comparator>& elem) {
    return std::visit( //
        overload {
            [](const op_link o) {
                switch (o) {
                    case op_link::and_: return "and";
                    case op_link::or_: return "or";
                    default: return "none";
                }
                std::unreachable();
            },
            [](const op_comparator o) {
                switch (o) {
                    case op_comparator::different: return "different";
                    case op_comparator::equal: return "equal";
                    case op_comparator::greater: return "greater";
                    case op_comparator::greater_equal: return "greater_equal";
                    case op_comparator::less: return "less";
                    case op_comparator::less_equal: return "less_equal";
                    default: return "none";
                }
                std::unreachable();
            },
        },
        elem);
}

} // namespace fil

TEST_CASE("Copa: mixed aggregator and ast_tree_generator", "[copa][single_run]") {
    SECTION("parse: simple variable with access") {
        fil::buffer_reader reader("obj.field == 42");

        language_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        std::println("{}", fil::to_string(result.value()));

        // Top level should be comparison (==)
        REQUIRE(std::holds_alternative<op_comparator>(result.value().value));
        CHECK(std::get<op_comparator>(result.value().value) == op_comparator::equal);

        // Left side should be a variable with access
        REQUIRE(std::holds_alternative<variable>(result.value().lhs));
        const auto lhs_var = std::get<variable>(result.value().lhs);
        CHECK(lhs_var.variable_name == "obj");
        REQUIRE(lhs_var.access.has_value());
        CHECK(lhs_var.access.value() == "field");

        // Right side should be a number
        REQUIRE(std::holds_alternative<int>(result.value().rhs));
        CHECK(std::get<int>(result.value().rhs) == 42);
    }

    SECTION("parse: simple double integer check") {
        fil::buffer_reader reader("2 && 10 ");

        language_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        std::println("{}", fil::to_string(result.value()));

        REQUIRE(std::holds_alternative<op_link>(result.value().value));
        CHECK(std::get<op_link>(result.value().value) == op_link::and_);

        REQUIRE(std::holds_alternative<int>(result.value().lhs));
        CHECK(std::get<int>(result.value().lhs) == 2);
        REQUIRE(std::holds_alternative<int>(result.value().rhs));
        CHECK(std::get<int>(result.value().rhs) == 10);
    }

    SECTION("parse: two variables with access compared") {
        fil::buffer_reader reader("user.age == admin.age");

        language_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        std::println("{}", fil::to_string(result.value()));

        REQUIRE(std::holds_alternative<op_comparator>(result.value().value));
        CHECK(std::get<op_comparator>(result.value().value) == op_comparator::equal);

        REQUIRE(std::holds_alternative<variable>(result.value().lhs));
        const auto lhs_var = std::get<variable>(result.value().lhs);
        CHECK(lhs_var.variable_name == "user");
        REQUIRE(lhs_var.access.has_value());
        CHECK(lhs_var.access.value() == "age");

        REQUIRE(std::holds_alternative<variable>(result.value().rhs));
        const auto rhs_var = std::get<variable>(result.value().rhs);
        CHECK(rhs_var.variable_name == "admin");
        REQUIRE(lhs_var.access.has_value());
        CHECK(rhs_var.access.value() == "age");
    }

    SECTION("parse: variable with access in logical AND") {
        fil::buffer_reader reader("user.active == 1 && user.banned != 1");

        language_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        std::println("{}", fil::to_string(result.value()));

        //            &&
        //          /    \
        //         ==     !=
        //       /   \    |   \
        //      /     1   |    1
        // user.active    user.banned
        //

        // Top level should be AND
        REQUIRE(std::holds_alternative<op_link>(result.value().value));
        CHECK(std::get<op_link>(result.value().value) == op_link::and_);

        // Left side: user.active == 1
        const auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs != nullptr);
        REQUIRE(std::holds_alternative<op_comparator>(lhs->value));
        CHECK(std::get<op_comparator>(lhs->value) == op_comparator::equal);

        const auto lhs_eq = std::get<variable>(lhs->lhs);
        CHECK(lhs_eq.variable_name == "user");
        CHECK(lhs_eq.access.has_value());
        CHECK(lhs_eq.access.value() == "active");

        CHECK(std::holds_alternative<int>(lhs->rhs));
        CHECK(std::get<int>(lhs->rhs) == 1);

        // Right side: user.banned != 1
        const auto rhs = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs != nullptr);
        REQUIRE(std::holds_alternative<op_comparator>(rhs->value));
        CHECK(std::get<op_comparator>(rhs->value) == op_comparator::different);

        const auto rhs_diff = std::get<variable>(rhs->lhs);
        CHECK(rhs_diff.variable_name == "user");
        CHECK(rhs_diff.access.has_value());
        CHECK(rhs_diff.access.value() == "banned");

        CHECK(std::holds_alternative<int>(rhs->rhs));
        CHECK(std::get<int>(lhs->rhs) == 1);
    }

    SECTION("parse: variable with access in logical OR") {
        // fil::buffer_reader reader("config.debug == 1 || config.verbose == 1");
        //
        // language_grammar g;
        // const auto result = fil::copa::parse(g, std::move(reader));
        // REQUIRE(result.has_value());
        //
        // CHECK(result.value().value == op_link::or_);
        //
        // auto lhs_eq = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        // REQUIRE(lhs_eq != nullptr);
        // CHECK(lhs_eq->value == op_comparator::equal);
        // auto lhs_var = std::get<std::shared_ptr<ast_node>>(lhs_eq->lhs);
        // auto lhs     = std::get<variable>(lhs_var->leaf);
        // CHECK(lhs.variable_name == "config");
        // CHECK(lhs.access.value() == "debug");
        //
        // auto rhs_eq = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        // REQUIRE(rhs_eq != nullptr);
        // CHECK(rhs_eq->value == op_comparator::equal);
        // auto rhs_var = std::get<std::shared_ptr<ast_node>>(rhs_eq->lhs);
        // auto rhs     = std::get<variable>(rhs_var->leaf);
        // CHECK(rhs.variable_name == "config");
        // CHECK(rhs.access.value() == "verbose");
    }

    // SECTION("parse: complex expression with variables and numbers") {
    //     fil::buffer_reader reader("data.threshold > 100 && status.code != 0");
    //
    //     language_grammar g;
    //     const auto result = fil::copa::parse(g, std::move(reader));
    //     REQUIRE(result.has_value());
    //     fil::copa::debug::print_ast_tree(result.value());
    //
    //     CHECK(result.value().value == op_link::and_);
    //
    //     auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
    //     REQUIRE(lhs != nullptr);
    //     CHECK(lhs->value == op_comparator::greater);
    //     auto lhs_var = std::get<std::shared_ptr<ast_node>>(lhs->lhs);
    //     auto var1    = std::get<variable>(lhs_var->leaf);
    //     CHECK(var1.variable_name == "data");
    //     CHECK(var1.access.value() == "threshold");
    //     CHECK(std::get<int>(lhs->rhs) == 100);
    //
    //     auto rhs = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
    //     REQUIRE(rhs != nullptr);
    //     CHECK(rhs->value == op_comparator::different);
    //     auto rhs_var = std::get<std::shared_ptr<ast_node>>(rhs->lhs);
    //     auto var2    = std::get<variable>(rhs_var->leaf);
    //     CHECK(var2.variable_name == "status");
    //     CHECK(var2.access.value() == "code");
    //     CHECK(std::get<int>(rhs->rhs) == 0);
    // }
    //
    // SECTION("parse: variable with access in parentheses") {
    //     fil::buffer_reader reader("(obj.value > 5) && count < 10");
    //
    //     language_grammar g;
    //     const auto result = fil::copa::parse(g, std::move(reader));
    //     REQUIRE(result.has_value());
    //     fil::copa::debug::print_ast_tree(result.value());
    //
    //     CHECK(result.value().value == op_link::and_);
    //
    //     auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
    //     REQUIRE(lhs != nullptr);
    //     // Parenthesised expression wrapped
    //     auto lhs_comparison = std::get<std::shared_ptr<ast_node>>(lhs->lhs);
    //     REQUIRE(lhs_comparison != nullptr);
    //     CHECK(lhs_comparison->value == op_comparator::greater);
    //     auto var = std::get<variable>(std::get<std::shared_ptr<ast_node>>(lhs_comparison->lhs)->leaf);
    //     CHECK(var.variable_name == "obj");
    //     CHECK(var.access.value() == "value");
    //     CHECK(std::get<int>(lhs_comparison->rhs) == 5);
    // }
    //
    // SECTION("parse: multiple variables with access in chain") {
    //     fil::buffer_reader reader("a.x > 5 || b.y < 10 || c.z == 0");
    //
    //     language_grammar g;
    //     const auto result = fil::copa::parse(g, std::move(reader));
    //     REQUIRE(result.has_value());
    //     fil::copa::debug::print_ast_tree(result.value());
    //
    //     // Top-level OR
    //     CHECK(std::holds_alternative<op_link>(result.value().value));
    //     CHECK(std::get<op_link>(result.value().value) == op_link::or_);
    //
    //     // Left: a.x > 5
    //     auto lhs_or = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
    //     REQUIRE(lhs_or != nullptr);
    //     CHECK(std::holds_alternative<op_comparator>(lhs_or->value));
    //     CHECK(std::get<op_comparator>(lhs_or->value) == op_comparator::greater);
    //
    //     CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(lhs_or->lhs));
    //     auto var_a = std::get<variable>(std::get<std::shared_ptr<ast_node>>(lhs_or->lhs));
    //     CHECK(var_a.variable_name == "a");
    //     CHECK(var_a.access.value() == "x");
    //
    //     // Right: nested OR
    //     auto rhs_or = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
    //     REQUIRE(rhs_or != nullptr);
    //     CHECK(rhs_or->value == op_link::or_);
    //     CHECK(std::get<std::shared_ptr<ast_node>>(rhs_or->lhs)->value == op_comparator::less);
    //     CHECK(std::get<std::shared_ptr<ast_node>>(rhs_or->rhs)->value == op_comparator::equal);
    // }
}