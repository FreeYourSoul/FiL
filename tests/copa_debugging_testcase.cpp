#include <catch2/catch_all.hpp>

#include "fil/copa/copa.hh"
#include "fil/copa/matcher.hh"
#include "fil/copa/member.hh"
#include "fil/copa/sink.hh"
#include "fil/copa/wrapper_utils.hh"
#include "fil/file/file_reader.hh"
#include "fil/file/temporary.hh"
#include "fil/meta/buffer_reader.hh"
#include "fil/meta/visit_utils.hh"

TEST_CASE("copa : debugging info in aggregator", "[copa]") {

    SECTION("concept check : has_debug_info") {
        struct with_debug {
            fil::copa::debug_info copa_debug_info;
        };
        struct wrong_type {
            std::string copa_debug_info; // wrong type, not convertible to debug_info
        };
        struct without_debug {
            std::string value;
        };

        static_assert(fil::copa::sink::debug_aggregation::with_debug_info_type<with_debug>);
        static_assert(!fil::copa::sink::debug_aggregation::with_debug_info_type<wrong_type>);
        static_assert(!fil::copa::sink::debug_aggregation::with_debug_info_type<without_debug>);
    }

    SECTION("aggregator: copa_debug_info populated after successful parse") {
        struct grammar_with_debug {
            struct ast_object {
                std::string value;
                fil::copa::debug_info copa_debug_info;
            };
            static constexpr auto rules() { return fil::copa::match_identifier<fil::copa::member<&ast_object::value>> {}; }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        fil::buffer_reader reader("hello ");
        grammar_with_debug grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));

        REQUIRE(result.has_value());
        CHECK(result->value == "hello");
        CHECK(result->copa_debug_info.line == 1);
        CHECK(result->copa_debug_info.cursor > 0);
    }

    SECTION("aggregator: no copa_debug_info member - no-op, parse still succeeds") {
        struct grammar_no_debug {
            struct ast_object {
                std::string value;
            };
            static constexpr auto rules() { return fil::copa::match_identifier<fil::copa::member<&ast_object::value>> {}; }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        fil::buffer_reader reader("hello ");
        grammar_no_debug grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));

        REQUIRE(result.has_value());
        CHECK(result->value == "hello");
    }

    SECTION("aggregator: copa_debug_info line reflects multiline input") {
        struct grammar_multiline {
            struct ast_object {
                std::string first;
                std::string second;
                fil::copa::debug_info copa_debug_info;
            };
            static constexpr auto rules() {
                return fil::copa::match_identifier<fil::copa::member<&ast_object::first>> {}
                     + fil::copa::match_identifier<fil::copa::member<&ast_object::second>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        fil::buffer_reader reader("foo\nbar ");
        grammar_multiline grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));

        REQUIRE(result.has_value());
        CHECK(result->first == "foo");
        CHECK(result->second == "bar");
        CHECK(result->copa_debug_info.line >= 2);
    }

    SECTION("ast_tree_generator: copa_debug_info populated after successful parse") {
        enum class op {
            INVALID,
            plus,
            minus
        };

        using ast_node_debug = fil::copa::ast_node<[](const std::string& token) -> op {
            if (token == "+")
                return op::plus;
            if (token == "-")
                return op::minus;
            return op::INVALID;
        }>;

        struct ast_node_with_debug : ast_node_debug {
            fil::copa::debug_info copa_debug_info;
        };

        struct operand_grammar {
            using ast_object = ast_node_debug;
            static constexpr auto rules() {
                return fil::copa::match_string<fil::fixed_string {"+"}, ast_node_debug::operand> {}
                     | fil::copa::match_string<fil::fixed_string {"-"}, ast_node_debug::operand> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::ast_tree_generator<ast_node_debug> {1}; }
        };

        struct base_grammar {
            using ast_object = ast_node_debug;
            static constexpr auto rules() { return fil::copa::match_number<ast_node_debug::leaf> {}; }
            static constexpr auto convertor() { return fil::copa::sink::ast_tree_generator<ast_node_debug> {0}; }
        };

        struct calc_grammar {
            using ast_object = ast_node_debug;
            static constexpr auto rules() {
                return fil::copa::list_rule<fil::copa::or_rule< //
                    fil::copa::match_parser<base_grammar>,      //
                    fil::copa::match_parser<operand_grammar>>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::ast_tree_generator<ast_node_debug> {0}; }
        };

        struct grammar_with_debug {
            struct ast_object {
                ast_node_debug node;
                fil::copa::debug_info copa_debug_info;
            };
            static constexpr auto rules() { return fil::copa::match_production<calc_grammar, fil::copa::member<&ast_object::node>> {}; }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        fil::buffer_reader reader("1 + 2 ");
        grammar_with_debug grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));

        REQUIRE(result.has_value());
        CHECK(result->copa_debug_info.line == 1);
        CHECK(result->copa_debug_info.cursor > 0);
    }
}

TEST_CASE("copa : debugging info in aggregator (file_reader)", "[copa]") {

    SECTION("aggregator: copa_debug_info populated when parsing from a file") {
        struct grammar_with_debug {
            struct ast_object {
                std::string value;
                fil::copa::debug_info copa_debug_info;
            };
            static constexpr auto rules() { return fil::copa::match_identifier<fil::copa::member<&ast_object::value>> {}; }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        const fil::temporary_file tmp("hello ");
        fil::file_reader reader(tmp);
        grammar_with_debug grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));

        REQUIRE(result.has_value());
        CHECK(result->value == "hello");
        CHECK(result->copa_debug_info.line == 1);
        CHECK(result->copa_debug_info.cursor > 0);
    }

    SECTION("aggregator: no copa_debug_info member - no-op, file parse still succeeds") {
        struct grammar_no_debug {
            struct ast_object {
                std::string value;
            };
            static constexpr auto rules() { return fil::copa::match_identifier<fil::copa::member<&ast_object::value>> {}; }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        const fil::temporary_file tmp("hello ");
        fil::file_reader reader(tmp);
        grammar_no_debug grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));

        REQUIRE(result.has_value());
        CHECK(result->value == "hello");
    }

    SECTION("aggregator: copa_debug_info line reflects multiline file") {
        struct grammar_multiline {
            struct ast_object {
                std::string first;
                std::string second;
                fil::copa::debug_info copa_debug_info;
            };
            static constexpr auto rules() {
                return fil::copa::match_identifier<fil::copa::member<&ast_object::first>> {}
                     + fil::copa::match_identifier<fil::copa::member<&ast_object::second>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        const fil::temporary_file tmp("foo\nbar ");
        fil::file_reader reader(tmp);
        grammar_multiline grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));

        REQUIRE(result.has_value());
        CHECK(result->first == "foo");
        CHECK(result->second == "bar");
        CHECK(result->copa_debug_info.line >= 2);
    }

    SECTION("aggregator: copa_debug_info cursor reflects position in file") {
        struct grammar_sequence {
            struct ast_object {
                std::string first;
                std::string second;
                std::string third;
                fil::copa::debug_info copa_debug_info;
            };
            static constexpr auto rules() {
                return fil::copa::match_identifier<fil::copa::member<&ast_object::first>> {}
                     + fil::copa::match_identifier<fil::copa::member<&ast_object::second>> {}
                     + fil::copa::match_identifier<fil::copa::member<&ast_object::third>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        const fil::temporary_file tmp("aa bb cc ");
        fil::file_reader reader(tmp);
        grammar_sequence grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));

        REQUIRE(result.has_value());
        CHECK(result->first == "aa");
        CHECK(result->second == "bb");
        CHECK(result->third == "cc");
        // cursor advanced past all three tokens
        CHECK(result->copa_debug_info.cursor >= 6);
    }
}
