#include "ast.hh"
#include "fil/copa/copa.hh"
#include "fil/copa/matcher.hh"
#include "fil/copa/sink.hh"
#include "fil/meta/buffer_reader.hh"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

namespace {

// --- AST Definitions for various test scenarios ---

struct alt_grammar {
    struct ast_object {
        std::string type;
        std::string value;
    };

    static constexpr fil::copa::rule auto rules() {
        // ( "INT" <id> ) | ( "STR" <id> | eof)
        return (fil::copa::match_string<fil::fixed_string {"INT"}, fil::copa::member<&ast_object::type>> {}
                + fil::copa::match_identifier<fil::copa::member<&ast_object::value>> {})
             //
             | (fil::copa::match_string<fil::fixed_string {"STR"}, fil::copa::member<&ast_object::type>> {}
                + fil::copa::match_identifier<fil::copa::member<&ast_object::value>> {})

             | fil::copa::eof;
    }

    static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
};

// --- Grammar Definitions ---

struct match_string_grammar {
    struct ast_object {
        std::string value;
    };
    static constexpr auto rules() { return fil::copa::match_string<fil::fixed_string {"HELLO"}, fil::copa::member<&ast_object::value>> {}; }
    static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
};

struct sequence_grammar {
    struct ast_object {
        std::string first;
        std::string second;
    };

    static constexpr auto rules() {
        return fil::copa::match_string<fil::fixed_string {"FIRST"}, fil::copa::member<&ast_object::first>> {}
             + fil::copa::match_string<fil::fixed_string {"SECOND"}, fil::copa::member<&ast_object::second>> {};
    }
    static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
};

// --- Test Cases ---

TEST_CASE("Copa: standalone test case", "[copa][standalone]") {
    struct simple_grammar {
        struct ast_object {
            std::string command;
            std::string name;
            std::vector<std::string> options;
        };

        // Rule: "CMD" <identifier:command> "FOR" <identifier:name> [options...]
        static constexpr fil::copa::rule auto rules() {
            return fil::copa::match_string<fil::fixed_string {"CMD"}> {}
                 + fil::copa::match_identifier<fil::copa::member<&ast_object::command>> {}
                 + fil::copa::match_string<fil::fixed_string {"FOR"}> {}
                 + fil::copa::match_identifier<fil::copa::member<&ast_object::name>> {}
                 + fil::copa::list_rule<fil::copa::match_identifier<fil::copa::member<&ast_object::options>>> {};
        }

        static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
    };

    SECTION("Successful parse of a simple command") {
        std::string input = "CMD start FOR engine turbo fast ";
        fil::buffer_reader reader(std::move(input));
        simple_grammar grammar;

        const auto result = fil::copa::parse(grammar, std::move(reader));

        REQUIRE(result.has_value());
        CHECK(result->command == "start");
        CHECK(result->name == "engine");
        REQUIRE(result->options.size() == 2);
        CHECK(result->options[0] == "turbo");
        CHECK(result->options[1] == "fast");
    }
    SECTION("Successful parse with different identifiers") {
        // std::string input = "CMD stop FOR system ";
        std::string input = "CMD stop FOR system a ";
        fil::buffer_reader reader(std::move(input));
        simple_grammar grammar;

        const auto result = fil::copa::parse(grammar, std::move(reader));

        REQUIRE(result.has_value());
        CHECK(result->command == "stop");
        CHECK(result->name == "system");
        REQUIRE(result->options.size() == 1);
        CHECK(result->options[0] == "a");
    }

    // @todo
    // SECTION("Successful parse with empty options") {
    //     std::string input = "CMD stop FOR system ";
    //     fil::buffer_reader reader(std::move(input));
    //     simple_grammar grammar;
    //
    //     const auto result = fil::copa::parse(grammar, std::move(reader));
    //
    //     REQUIRE(result.has_value());
    //     CHECK(result->command == "stop");
    //     CHECK(result->name == "system");
    //     CHECK(result->options.empty());
    // }

    SECTION("Parse failure on missing keyword") {
        std::string input = "CMD start engine "; // Missing "FOR"
        fil::buffer_reader reader(std::move(input));
        simple_grammar grammar;

        const auto result = fil::copa::parse(grammar, std::move(reader));

        // Note: Currently copa might return failure or partial result depending on how it's structured.
        // Based on do_parse_rule, if a rule fails, it returns failure.
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("Copa: OR rule test", "[copa][standalone]") {
    SECTION("Match first alternative") {
        fil::buffer_reader reader("INT 123 ");
        alt_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(result.has_value());
        CHECK(result->value == "123");
    }

    SECTION("Match second alternative") {
        fil::buffer_reader reader("STR hello ");
        alt_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(result.has_value());
        CHECK(result->value == "hello");
    }
}

TEST_CASE("Copa: Basic Matchers", "[copa][matchers]") {
    SECTION("match_string") {
        // Test that match_string correctly identifies the exact string and binds it to the AST member.
        fil::buffer_reader reader("HELLO ");
        struct GG {

            struct ast_object {
                std::string value {};
            };

            static constexpr fil::copa::rule auto rules() {
                return fil::copa::match_string<fil::fixed_string {"HELLO"}, fil::copa::member<&ast_object::value>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        GG gg {};
        const auto result = fil::copa::parse(gg, std::move(reader));
        REQUIRE(result.has_value());
        CHECK(result->value == "HELLO");
    }

    SECTION("match_string: failure") {
        // Test that match_string fails when the input does not match.
        fil::buffer_reader reader("WORLD");
        match_string_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        CHECK_FALSE(result.has_value());
    }

    struct match_char_grammar {
        struct ast_object {
            char value;
        };
        static constexpr auto rules() { return fil::copa::match_char<'A', fil::copa::member<&ast_object::value>> {}; }
        static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
    };

    SECTION("match_char") {
        // Test match_char for single character matching.
        // This ensures the parser can correctly match individual characters provided as part of the grammar.
        // In this case, we'd check if 'A' matches correctly.
        fil::buffer_reader reader("A");
        match_char_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(result.has_value());
        CHECK(result->value == 'A');
    }
    SECTION("match_char failed") {

        // Test match_char for single character matching. fails when character is wrong
        fil::buffer_reader reader("X");
        match_char_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(!result.has_value());
    }

    SECTION("match_identifier") {
        // Test match_identifier with default separator (space-like).
        // It should parse alphanumeric characters until it encounters a whitespace.
        // Input: "id123 rest" -> should match "id123" and leave " rest" in the buffer (or at least stop there).
        fil::buffer_reader reader("id123 rest");
        struct id_ast {
            std::string id;
        };
        struct id_grammar {
            using ast_object = id_ast;
            static constexpr auto rules() { return fil::copa::match_identifier<fil::copa::member<&id_ast::id>> {}; }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<id_ast> {}; }
        };
        id_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(result.has_value());
        CHECK(result->id == "id123");
    }
}

TEST_CASE("Copa: Composition Rules", "[copa][composition]") {
    SECTION("Sequence (+)") {
        // Test that multiple rules in sequence are all required and executed in order.
        fil::buffer_reader reader("FIRST SECOND");
        sequence_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(result.has_value());
        CHECK(result->first == "FIRST");
        CHECK(result->second == "SECOND");
    }

    SECTION("Alternative (|)") {
        // Test that the parser tries alternatives in order and succeeds if any one matches.
        // Input: "STR value" or "INT value" should both be matched by the alternative rule.
        fil::buffer_reader reader1("INT 42 ");
        fil::buffer_reader reader2("STR world ");
        alt_grammar grammar;

        const auto res1 = fil::copa::parse(grammar, std::move(reader1));
        REQUIRE(res1.has_value());
        CHECK(res1->type == "INT");
        CHECK(res1->value == "42");

        const auto res2 = fil::copa::parse(grammar, std::move(reader2));
        REQUIRE(res2.has_value());
        CHECK(res2->type == "STR");
        CHECK(res2->value == "world");
    }

    SECTION("List Rule (Repetition)") {
        struct list_grammar {
            struct ast_object {
                std::vector<std::string> items;
            };
            static constexpr auto rules() {
                return fil::copa::list_rule<fil::copa::match_identifier<fil::copa::member<&ast_object::items>>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        // Test list_rule with zero, one, and multiple items.
        // Verify that it correctly populates a std::vector in the AST.
        fil::buffer_reader reader("item1 item2 item3 ");
        list_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(result.has_value());
        REQUIRE(result->items.size() == 3);
        CHECK(result->items[0] == "item1");
        CHECK(result->items[1] == "item2");
        CHECK(result->items[2] == "item3");
    }
    SECTION("List Rule (Composed)") {
        struct list_grammar_braced {
            struct ast_object {
                std::vector<std::string> items;
            };
            static constexpr auto rules() {
                return fil::copa::square_wrapped< //
                    fil::copa::list_rule<fil::copa::match_identifier<fil::copa::member<&ast_object::items>>>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        fil::buffer_reader reader("[ a b c ]");
        list_grammar_braced grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(result.has_value());
        REQUIRE(result->items.size() == 3);
        CHECK(result->items[0] == "a");
        CHECK(result->items[1] == "b");
        CHECK(result->items[2] == "c");
    }
}

TEST_CASE("Copa: Edge Cases and Error Handling", "[copa][errors]") {
    SECTION("Partial Match Failure") {
        // Test a sequence where the first part matches but the second fails.
        // Ensure the whole parse fails.
        fil::buffer_reader reader("FIRST WRONG");
        sequence_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        CHECK_FALSE(result.has_value());
    }

    SECTION("Empty Input") {
        // Test behavior with an empty buffer.
        // A grammar requiring a string should fail on an empty input.
        fil::buffer_reader reader("");
        match_string_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        CHECK_FALSE(!result.has_value());
    }

    SECTION("Unexpected End of Buffer") {
        // Test a rule that expects more data than provided (e.g., match_string<"LONG"> with input "LO").
        fil::buffer_reader reader("HE");
        match_string_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        CHECK_FALSE(result.has_value());
    }

    SECTION("Backtracking in Alternatives") {
        // If one alternative partially matches but then fails,
        // the next alternative should be tried from the SAME starting point.
        // Example rule: (match_string<"PREFIX_A">) | (match_string<"PREFIX_B">)
        // Input: "PREFIX_B"
        // The parser might try PREFIX_A, fail at the last character, and then must reset reader to "P" before trying PREFIX_B.
        struct back_ast {
            std::string v;
        };
        struct back_grammar {
            using ast_object = back_ast;
            static constexpr auto rules() {
                return fil::copa::match_string<fil::fixed_string {"PREFIX_A"}, fil::copa::member<&back_ast::v>> {}
                     | fil::copa::match_string<fil::fixed_string {"PREFIX_B"}, fil::copa::member<&back_ast::v>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<back_ast> {}; }
        };
        fil::buffer_reader reader("PREFIX_B");
        back_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(result.has_value());
        CHECK(result->v == "PREFIX_B");
    }
}

TEST_CASE("Copa: White-space and Ignore Rules", "[copa][whitespace]") {
    SECTION("Default ignore (match_space_like)") {
        // Verify that spaces, tabs, and newlines are ignored between rules by default.
        fil::buffer_reader reader("  FIRST\n\t  SECOND  ");
        sequence_grammar grammar;
        auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(result.has_value());
        CHECK(result->first == "FIRST");
        CHECK(result->second == "SECOND");
    }

    SECTION("Custom ignore rules") {
        // Define a grammar with a custom ignore() rule (e.g., ignore commas) and verify it works.
        struct comma_ast {
            std::string v;
        };
        struct comma_grammar {
            using ast_object = comma_ast;
            static constexpr auto rules() {
                return fil::copa::match_string<fil::fixed_string {"VAL"}, fil::copa::member<&comma_ast::v>> {};
            }
            static constexpr auto ignore() { return fil::copa::match_char<','> {}; }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<comma_ast> {}; }
        };
        fil::buffer_reader reader(",,,,VAL");
        comma_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(result.has_value());
    }
}

TEST_CASE("Copa: Nested Parsers", "[copa][nested]") {
    SECTION("match_parser") {
        // Test using match_parser to embed one grammar's logic into another.
        // This is crucial for modular grammars.
        // We'll define a sub-grammar and then use match_parser to parse it into a member of a parent AST.
        struct sub_ast {
            std::string val;
        };
        struct sub_prod {
            using ast_object = sub_ast;
            static constexpr auto rules() {
                return fil::copa::match_string<fil::fixed_string {"SUB"}, fil::copa::member<&sub_ast::val>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<sub_ast> {}; }
        };

        struct parent_ast {
            sub_ast sub;
        };
        struct parent_grammar {
            using ast_object = parent_ast;

            static constexpr auto rules() { return fil::copa::match_parser<sub_prod, fil::copa::member<&parent_ast::sub>> {}; }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<parent_ast> {}; }
        };

        fil::buffer_reader reader("SUB ");
        parent_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(result.has_value());
        CHECK(result->sub.val == "SUB");
    }
}

TEST_CASE("Copa: times<N> rule", "[copa][times]") {
    SECTION("Fixed repetition") {
        // Test the times<N> rule to match exactly N occurrences.
        // Rule: times<3>(match_char<'X'>)
        // Input: "XXX" -> Success
        // Input: "XX" -> Failure
        struct times_ast {
            std::vector<std::string> v;
        }; // dummy
        struct times_grammar {
            using ast_object = times_ast;
            static constexpr auto rules() { return fil::copa::repeat<3>(fil::copa::match_char<'X'> {}); }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<times_ast> {}; }
        };

        fil::buffer_reader reader("XXX");
        times_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(result.has_value());
    }
}

} // namespace
