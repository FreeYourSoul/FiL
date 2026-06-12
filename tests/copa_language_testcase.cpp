#include <catch2/catch_all.hpp>

#include "fil/copa/copa.hh"
#include "fil/copa/member.hh"
#include "fil/copa/sink.hh"
#include "fil/copa/visit.hh"
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

TEST_CASE("Copa: mixed aggregator and ast_tree_generator", "[copa]") {
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
        fil::buffer_reader reader("config.debug == 1337 || config.verbose == 42");

        language_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());

        std::println("{}", fil::to_string(result.value()));

        //            ||
        //          /    \
        //         ==      ==
        //       /   \    |   \
        //      /   1337  |    1
        // user.debug    user.verbose
        //

        REQUIRE(std::holds_alternative<op_link>(result.value().value));
        CHECK(std::get<op_link>(result.value().value) == op_link::or_);

        const auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs != nullptr);
        REQUIRE(std::holds_alternative<op_comparator>(lhs->value));
        CHECK(std::get<op_comparator>(lhs->value) == op_comparator::equal);

        REQUIRE(std::holds_alternative<variable>(lhs->lhs));
        const auto lhs1_var = std::get<variable>(lhs->lhs);
        CHECK(lhs1_var.variable_name == "config");
        REQUIRE(lhs1_var.access.has_value());
        CHECK(lhs1_var.access.value() == "debug");

        REQUIRE(std::holds_alternative<int>(lhs->rhs));
        CHECK(std::get<int>(lhs->rhs) == 1337);

        const auto rhs = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs != nullptr);
        REQUIRE(std::holds_alternative<op_comparator>(rhs->value));
        CHECK(std::get<op_comparator>(rhs->value) == op_comparator::equal);
        REQUIRE(std::holds_alternative<variable>(rhs->lhs));
        const auto lhs2_var = std::get<variable>(rhs->lhs);
        CHECK(lhs2_var.variable_name == "config");
        REQUIRE(lhs2_var.access.has_value());
        CHECK(lhs2_var.access.value() == "verbose");

        REQUIRE(std::holds_alternative<int>(rhs->rhs));
        CHECK(std::get<int>(rhs->rhs) == 42);
    }

    SECTION("parse: complex expression with variables and numbers") {
        fil::buffer_reader reader("data.threshold > 100 && 0 != status.code");

        language_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        std::println("{}", fil::to_string(result.value()));

        //            &&
        //          /    \
        //         >       !=
        //       /   \    |   \
        //      /    100   \    status.code
        // data.threshold   0
        //

        REQUIRE(std::holds_alternative<op_link>(result.value().value));
        CHECK(std::get<op_link>(result.value().value) == op_link::and_);

        REQUIRE(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().lhs));
        auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs != nullptr);
        REQUIRE(std::holds_alternative<op_comparator>(lhs->value));
        CHECK(std::get<op_comparator>(lhs->value) == op_comparator::greater);

        REQUIRE(std::holds_alternative<variable>(lhs->lhs));
        const auto lhs_var = std::get<variable>(lhs->lhs);
        CHECK(lhs_var.variable_name == "data");
        REQUIRE(lhs_var.access.has_value());
        CHECK(lhs_var.access.value() == "threshold");
        REQUIRE(std::holds_alternative<int>(lhs->rhs));
        CHECK(std::get<int>(lhs->rhs) == 100);

        REQUIRE(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().rhs));
        const auto rhs = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs != nullptr);
        CHECK(std::holds_alternative<op_comparator>(rhs->value));
        CHECK(std::get<op_comparator>(rhs->value) == op_comparator::different);

        REQUIRE(std::holds_alternative<variable>(rhs->rhs));
        const auto rhs_var = std::get<variable>(rhs->rhs);
        CHECK(rhs_var.variable_name == "status");
        CHECK(rhs_var.access.value() == "code");

        REQUIRE(std::holds_alternative<int>(rhs->lhs));
        CHECK(std::get<int>(rhs->lhs) == 0);
    }

    SECTION("parse: variable with access in parentheses") {
        fil::buffer_reader reader("(obj.value > 5 || 42) && count < 10");

        language_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        std::println("{}", fil::to_string(result.value()));

        REQUIRE(std::holds_alternative<op_link>(result.value().value));
        CHECK(std::get<op_link>(result.value().value) == op_link::and_);

        REQUIRE(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().lhs));
        const auto lhs = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs != nullptr);

        REQUIRE(std::holds_alternative<op_link>(lhs->value));
        CHECK(std::get<op_link>(lhs->value) == op_link::or_);

        // Parenthesised expression wrapped LHS
        const auto lhs_parenthesis_expr = std::get<std::shared_ptr<ast_node>>(lhs->lhs);
        REQUIRE(lhs_parenthesis_expr != nullptr);
        REQUIRE(std::holds_alternative<op_comparator>(lhs_parenthesis_expr->value));
        CHECK(std::get<op_comparator>(lhs_parenthesis_expr->value) == op_comparator::greater);

        REQUIRE(std::holds_alternative<variable>(lhs_parenthesis_expr->lhs));
        const auto rhs_var = std::get<variable>(lhs_parenthesis_expr->lhs);
        CHECK(rhs_var.variable_name == "obj");
        CHECK(rhs_var.access.has_value());
        CHECK(rhs_var.access.value() == "value");

        REQUIRE(std::holds_alternative<int>(lhs_parenthesis_expr->rhs));
        CHECK(std::get<int>(lhs_parenthesis_expr->rhs) == 5);

        REQUIRE(std::holds_alternative<int>(lhs->rhs));
        CHECK(std::get<int>(lhs->rhs) == 42);

        // RHS
        REQUIRE(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().rhs));
        const auto rhs = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs != nullptr);

        REQUIRE(std::holds_alternative<variable>(rhs->lhs));
        const auto rhs_lhs_less_var = std::get<variable>(rhs->lhs);
        CHECK(rhs_lhs_less_var.variable_name == "count");
        CHECK(!rhs_lhs_less_var.access.has_value());

        REQUIRE(std::holds_alternative<int>(rhs->rhs));
        CHECK(std::get<int>(rhs->rhs) == 10);
    }

    SECTION("parse: multiple variables with access in chain") {
        fil::buffer_reader reader("a.x > 5 || b.y < 10 || c.z == chocobo");

        language_grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        std::println("{}", fil::to_string(result.value()));

        //            ||
        //         /      \
        //        ||        ==
        //      /    \     /  \
        //    >       <   c.z  chocobo
        //   / \     / \
        // a.x  5  b.y  10

        // Top-level OR
        CHECK(std::holds_alternative<op_link>(result.value().value));
        CHECK(std::get<op_link>(result.value().value) == op_link::or_);

        // Left: a.x > 5 || b.y < 10
        REQUIRE(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().lhs));
        auto lhs_or = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(lhs_or != nullptr);
        CHECK(std::holds_alternative<op_link>(lhs_or->value));
        CHECK(std::get<op_link>(lhs_or->value) == op_link::or_);

        // Left_Left : a.x > 5
        auto lhs_left = std::get<std::shared_ptr<ast_node>>(lhs_or->lhs);
        REQUIRE(lhs_left != nullptr);
        REQUIRE(std::holds_alternative<op_comparator>(lhs_left->value));
        CHECK(std::get<op_comparator>(lhs_left->value) == op_comparator::greater);

        REQUIRE(std::holds_alternative<variable>(lhs_left->lhs));
        const auto axVar = std::get<variable>(lhs_left->lhs);
        CHECK(axVar.variable_name == "a");
        REQUIRE(axVar.access.has_value());
        CHECK(axVar.access.value() == "x");

        REQUIRE(std::holds_alternative<int>(lhs_left->rhs));
        CHECK(std::get<int>(lhs_left->rhs) == 5);

        // Left_Right : b.y < 10
        REQUIRE(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().rhs));
        auto lhs_right = std::get<std::shared_ptr<ast_node>>(lhs_or->rhs);
        REQUIRE(lhs_right != nullptr);
        REQUIRE(std::holds_alternative<op_comparator>(lhs_right->value));
        CHECK(std::get<op_comparator>(lhs_right->value) == op_comparator::less);

        REQUIRE(std::holds_alternative<variable>(lhs_right->lhs));
        const auto byVar = std::get<variable>(lhs_right->lhs);
        CHECK(byVar.variable_name == "b");
        REQUIRE(byVar.access.has_value());
        CHECK(byVar.access.value() == "y");

        REQUIRE(std::holds_alternative<int>(lhs_right->rhs));
        CHECK(std::get<int>(lhs_right->rhs) == 10);

        // Right : c.z == chocobo
        REQUIRE(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().rhs));
        auto rhs = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(rhs != nullptr);
        REQUIRE(std::holds_alternative<op_comparator>(rhs->value));
        CHECK(std::get<op_comparator>(rhs->value) == op_comparator::equal);

        REQUIRE(std::holds_alternative<variable>(rhs->lhs));
        const auto czVar = std::get<variable>(rhs->lhs);
        CHECK(czVar.variable_name == "c");
        REQUIRE(czVar.access.has_value());
        CHECK(czVar.access.value() == "z");

        REQUIRE(std::holds_alternative<variable>(rhs->rhs));
        const auto chocobo = std::get<variable>(rhs->rhs);
        CHECK(chocobo.variable_name == "chocobo");
        CHECK(!chocobo.access.has_value());
    }
}

// define outside inner-scope to provide visitor specialization
struct agg_language {
    struct ast_object {
        std::vector<ast_node> nodes;

        std::string to_string() const { return fil::join(nodes, "\n===========\n"); }
    };

    static constexpr auto rules() {
        return fil::copa::repeat<2>(                                                                 //
            (fil::copa::match_production<language_grammar, fil::copa::member<&ast_object::nodes>> {} //
             + fil::copa::comma));
    }
    static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
};

// specialize visitation
namespace fil::copa {

template<typename Callback>
auto visit(Callback&& callback, const agg_language::ast_object& obj) {
    for (const auto& n : obj.nodes)
        fil::copa::visit(callback, n);
    return 0.0;
}

} // namespace fil::copa

TEST_CASE("Copa: aggregation language", "[copa]") {

    fil::buffer_reader reader(" chocobo.fly >= 5, c < 0, ");

    agg_language g;
    const auto result = fil::copa::parse(g, std::move(reader));
    REQUIRE(result.has_value());
    std::println("{}", fil::to_string(result.value()));

    REQUIRE(result->nodes.size() == 2);

    SECTION("test combination of aggregation with ast tree") {

        const auto& node0 = result->nodes[0];
        const auto& node1 = result->nodes[1];

        // Top level should be comparison (==)
        REQUIRE(std::holds_alternative<op_comparator>(node0.value));
        CHECK(std::get<op_comparator>(node0.value) == op_comparator::greater_equal);

        const auto& lhs0 = node0.lhs;
        const auto& rhs0 = node0.rhs;

        // Left side should be a variable with access
        REQUIRE(std::holds_alternative<variable>(lhs0));
        const auto lhs_var = std::get<variable>(lhs0);
        CHECK(lhs_var.variable_name == "chocobo");
        REQUIRE(lhs_var.access.has_value());
        CHECK(lhs_var.access.value() == "fly");

        REQUIRE(std::holds_alternative<int>(rhs0));
        CHECK(std::get<int>(rhs0) == 5);

        REQUIRE(std::holds_alternative<op_comparator>(node1.value));
        CHECK(std::get<op_comparator>(node1.value) == op_comparator::less);

        const auto& lhs1 = node1.lhs;
        const auto& rhs1 = node1.rhs;

        REQUIRE(std::holds_alternative<variable>(lhs1));
        const auto lhs_var1 = std::get<variable>(lhs1);
        CHECK(lhs_var1.variable_name == "c");
        CHECK(!lhs_var1.access.has_value());

        REQUIRE(std::holds_alternative<int>(rhs1));
        CHECK(std::get<int>(rhs1) == 0);
    }

    SECTION("test visit on aggregation") {

        struct final_visitation_result {
            std::vector<variable> variables_used;
            std::vector<op_link> oplink_used;
            std::vector<op_comparator> opcomp_used;
            std::vector<int> values_used;
        };

        final_visitation_result res;
        const auto node_visitor_with_return = fil::overload {
            [](const auto&, const auto&) { return 0.0; },
            [&res](const auto&, const variable& var) {
                res.variables_used.push_back(var);
                return 0.0;
            },
            [&res](const auto&, int i) {
                res.values_used.push_back(i);
                return 0.0;
            },
            [&res](const auto&, const std::variant<op_link, op_comparator>& op_variant) {
                // Handle the variant - visit it to extract the actual enum value
                std::visit(
                    [&res](const auto& op) {
                        using T = std::decay_t<decltype(op)>;
                        if constexpr (std::is_same_v<T, op_comparator>) {
                            res.opcomp_used.push_back(op);
                        } else if constexpr (std::is_same_v<T, op_link>) {
                            res.oplink_used.push_back(op);
                        }
                    },
                    op_variant);
                return 0.0;
            },
        };
        const auto& result_visit = fil::copa::visit(node_visitor_with_return, result.value());

        REQUIRE(res.variables_used.size() == 2);
        REQUIRE(res.oplink_used.size() == 0);
        REQUIRE(res.opcomp_used.size() == 2);
        REQUIRE(res.values_used.size() == 2);

        CHECK(res.variables_used[0].variable_name == "chocobo");
        CHECK(res.variables_used[0].access.value() == "fly");
        CHECK(res.variables_used[1].variable_name == "c");
        CHECK(!res.variables_used[1].access.has_value());

        CHECK(res.opcomp_used[0] == op_comparator::greater_equal);
        CHECK(res.opcomp_used[1] == op_comparator::less);

        CHECK(res.values_used[0] == 5);
        CHECK(res.values_used[1] == 0);
    }
}
