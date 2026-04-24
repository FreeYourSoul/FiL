#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <string>
#include <vector>

#include "ast.hh"
#include "debug.hh"
#include "fil/copa/copa.hh"
#include "fil/copa/matcher.hh"
#include "fil/copa/sink.hh"
#include "fil/file/file_reader.hh"
#include "fil/file/temporary.hh"
#include "fil/meta/buffer_reader.hh"
#include "print_error.hh"

namespace {

template<typename T>
struct s {
    using ast_object = T::ast_object;

    static constexpr auto rules() { return T::rules(); }
    static constexpr auto convertor() { return T::convert(); }
};

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
                 + fil::copa::may_rule<fil::copa::list_rule<fil::copa::match_identifier<fil::copa::member<&ast_object::options>>>> {};
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

    SECTION("Successful parse with empty options") {
        std::string input = "CMD stop FOR system ]";
        fil::buffer_reader reader(std::move(input));
        simple_grammar grammar;

        const auto result = fil::copa::parse(grammar, std::move(reader));

        REQUIRE(result.has_value());
        CHECK(result->command == "stop");
        CHECK(result->name == "system");
        CHECK(result->options.empty());
    }

    SECTION("Parse failure on missing keyword") {
        std::string input = "CMD start engine "; // Missing "FOR"
        fil::buffer_reader reader(std::move(input));
        simple_grammar grammar;

        const auto result = fil::copa::parse(grammar, std::move(reader));

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

    SECTION("Match failure") {
        fil::buffer_reader reader("INT ");
        alt_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(!result.has_value());
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

    SECTION("match_number : success") {
        struct match_integer_grammar {
            struct ast_object {
                std::string type;
                int value;
            };
            static constexpr auto rules() {
                return                                                                                          //
                    fil::copa::match_string<fil::fixed_string {"INT"}, fil::copa::member<&ast_object::type>> {} //
                    + fil::copa::match_number<fil::copa::member<&ast_object::value>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        SECTION("match failure not-integer") {
            auto grammar = match_integer_grammar {};
            fil::buffer_reader reader("INT chocobo ");
            const auto result = fil::copa::parse(grammar, std::move(reader));
            REQUIRE(!result.has_value());

            fil::buffer_reader r("INT chocobo ");
            fil::copa::print_error(r, result.error().get_errors().back());
        }

        SECTION("match single integer") {

            auto grammar = match_integer_grammar {};
            fil::buffer_reader reader("INT 4242 ");
            const auto result = fil::copa::parse(grammar, std::move(reader));
            REQUIRE(result.has_value());
            CHECK(result->type == "INT");
            CHECK(result->value == 4242);
        }

        SECTION("match for vector of integer") {
            struct match_integers_grammar {
                struct ast_object {
                    std::string type;
                    std::vector<int> values;
                };
                static constexpr auto rules() {
                    return                                                                                          //
                        fil::copa::match_string<fil::fixed_string {"INT"}, fil::copa::member<&ast_object::type>> {} //
                        + fil::copa::repeat<4>(fil::copa::match_number<fil::copa::member<&ast_object::values>> {});
                }
                static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
            };

            auto grammar = match_integers_grammar {};
            fil::buffer_reader reader("INT 4242 0 1 1337 ");
            const auto result = fil::copa::parse(grammar, std::move(reader));
            REQUIRE(result.has_value());

            CHECK(result->type == "INT");
            REQUIRE(result->values.size() == 4);
            CHECK(result->values[0] == 4242);
            CHECK(result->values[1] == 0);
            CHECK(result->values[2] == 1);
            CHECK(result->values[3] == 1337);
        }
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
    SECTION("match_char multiple") {
        fil::buffer_reader reader("CHOCOBO");

        struct match_chocobo_char_grammar {
            struct ast_object {
                char c1;
                char c2;
                char c3;
                char c4;
                char c5;
                char c6;
                char c7;
            };
            static constexpr auto rules() {
                return                                                                  //
                    fil::copa::match_char<'C', fil::copa::member<&ast_object::c1>> {}   //
                    + fil::copa::match_char<'H', fil::copa::member<&ast_object::c2>> {} //
                    + fil::copa::match_char<'O', fil::copa::member<&ast_object::c3>> {} //
                    + fil::copa::match_char<'C', fil::copa::member<&ast_object::c4>> {} //
                    + fil::copa::match_char<'O', fil::copa::member<&ast_object::c5>> {} //
                    + fil::copa::match_char<'B', fil::copa::member<&ast_object::c6>> {} //
                    + fil::copa::match_char<'O', fil::copa::member<&ast_object::c7>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        match_chocobo_char_grammar grammar {};
        const auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(result.has_value());
        CHECK(result->c1 == 'C');
        CHECK(result->c2 == 'H');
        CHECK(result->c3 == 'O');
        CHECK(result->c4 == 'C');
        CHECK(result->c5 == 'O');
        CHECK(result->c6 == 'B');
        CHECK(result->c7 == 'O');
    }
    SECTION("match_char multiple (with setter)") {
        fil::buffer_reader reader("CHOCOBO");

        struct match_chocobo_char_grammar {
            struct ast_object {
                std::string value;

                void append_value(char c) { value.push_back(c); }
            };
            static constexpr auto rules() {
                return                                                                            //
                    fil::copa::match_char<'C', fil::copa::member<&ast_object::append_value>> {}   //
                    + fil::copa::match_char<'H', fil::copa::member<&ast_object::append_value>> {} //
                    + fil::copa::match_char<'O', fil::copa::member<&ast_object::append_value>> {} //
                    + fil::copa::match_char<'C', fil::copa::member<&ast_object::append_value>> {} //
                    + fil::copa::match_char<'O', fil::copa::member<&ast_object::append_value>> {} //
                    + fil::copa::match_char<'B', fil::copa::member<&ast_object::append_value>> {} //
                    + fil::copa::match_char<'O', fil::copa::member<&ast_object::append_value>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        match_chocobo_char_grammar grammar {};
        const auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(result.has_value());
        CHECK(result->value == "CHOCOBO");
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
        std::string content {"\nFIRST WRONG\nd\n"};
        fil::buffer_reader reader(content);
        sequence_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        CHECK_FALSE(result.has_value());

        const auto f1 = fil::temporary_file(content);
        fil::file_reader file_reader {f1};
        fil::copa::print_error(file_reader, result.error().get_errors().back());
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

    SECTION("Nested list grammar") {
        struct item_ast {
            std::string name;
            std::string type;
        };

        struct item_grammar {
            using ast_object = item_ast;

            static constexpr auto rules() {
                return fil::copa::match_identifier<fil::copa::member<&ast_object::name>> {} //
                     + fil::copa::double_point                                              //
                     + fil::copa::match_identifier<fil::copa::member<&ast_object::type>> {} //
                     + fil::copa::semicol;
            }

            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        struct nested_list_grammar {

            struct ast_object {
                std::vector<item_ast> items; // List of complex structures
            };

            static constexpr auto rules() {
                return fil::copa::list_rule<fil::copa::match_parser<item_grammar, fil::copa::member<&ast_object::items>>> {};
            }

            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        SECTION("item grammar check") {
            fil::buffer_reader reader("a:int;");
            item_grammar grammar;
            const auto result = fil::copa::parse(grammar, std::move(reader));
            REQUIRE(result.has_value());
            CHECK(result->name == "a");
            CHECK(result->type == "int");
        }

        SECTION("zero element") {
            auto content = GENERATE("", " ; ", "a : int", " :int;", ":;");

            fil::buffer_reader reader(content);
            nested_list_grammar grammar;
            const auto result = fil::copa::parse(grammar, std::move(reader));
            REQUIRE(result.has_value());
            CHECK(result->items.empty());
        }

        SECTION("single element") {
            auto content = GENERATE( //
                "a:int;",            //
                "a   :int; ",        //
                "a   :    int \t\n;");

            fil::buffer_reader reader(content);
            nested_list_grammar grammar;
            const auto result = fil::copa::parse(grammar, std::move(reader));
            REQUIRE(result.has_value());
            REQUIRE(result->items.size() == 1);
            CHECK(result->items[0].name == "a");
            CHECK(result->items[0].type == "int");
        }

        SECTION("multiple element") {
            fil::buffer_reader reader("a : int ;  b:string   ; c :   vector ;  ");
            nested_list_grammar grammar;
            const auto result = fil::copa::parse(grammar, std::move(reader));
            REQUIRE(result.has_value());
            REQUIRE(result->items.size() == 3);
            CHECK(result->items[0].name == "a");
            CHECK(result->items[0].type == "int");
            CHECK(result->items[1].name == "b");
            CHECK(result->items[1].type == "string");
            CHECK(result->items[2].name == "c");
            CHECK(result->items[2].type == "vector");
        }
    }

    SECTION("Recursive : Nested") {
        struct grammar_recursive_nested {
            struct ast_object {
                std::vector<std::string> datas;

                void concat(ast_object&& obj) {
                    datas.insert(datas.end(), std::make_move_iterator(obj.datas.begin()), std::make_move_iterator(obj.datas.end()));
                }
            };

            static constexpr auto rules() {
                return fil::copa::list( //
                    fil::copa::match_identifier<fil::copa::member<&ast_object::datas>> {}
                    | fil::copa::parenthesised(
                        fil::copa::match_parser<grammar_recursive_nested, fil::copa::member<&ast_object::concat>> {}));
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        static_assert(fil::copa::production<grammar_recursive_nested>);

        SECTION("simple") {
            fil::buffer_reader reader("chocobo (run)");
            grammar_recursive_nested g;
            const auto result = fil::copa::parse(g, std::move(reader));

            REQUIRE(result.has_value());
            REQUIRE(result->datas.size() == 2);
            CHECK(result->datas[0] == "chocobo");
            CHECK(result->datas[1] == "run");
        }

        SECTION("more complext") {

            fil::buffer_reader reader("chocobo (cannot fly (they just) run)");
            grammar_recursive_nested g;
            const auto result = fil::copa::parse(g, std::move(reader));

            REQUIRE(result.has_value());
            REQUIRE(result->datas.size() == 6);
            CHECK(result->datas[0] == "chocobo");
            CHECK(result->datas[1] == "cannot");
            CHECK(result->datas[2] == "fly");
            CHECK(result->datas[3] == "they");
            CHECK(result->datas[4] == "just");
            CHECK(result->datas[5] == "run");
        }
    }

    SECTION("Deeply Nested Parameters") {
        struct level3_ast {
            std::string data;
        };

        struct level2_ast {
            level3_ast nested;
        };

        struct level1_ast {
            std::string data;
            std::vector<level2_ast> nested;
        };
        struct level3_grammar {
            using ast_object = level3_ast;

            static constexpr auto rules() {           //
                return fil::copa::apostrophe_wrapped< //
                    fil::copa::match_identifier<fil::copa::member<&ast_object::data>>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        struct level2_grammar {
            using ast_object = level2_ast;

            static constexpr auto rules() { //
                return fil::copa::match_string<fil::fixed_string {"data:"}> {}
                     + fil::copa::match_parser<level3_grammar, fil::copa::member<&ast_object::nested>> {};
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        struct level1_grammar {
            using ast_object = level1_ast;

            // "struct:" + name + "->" + list_of<level2> + ";"
            static constexpr auto rules() { //
                return fil::copa::match_string<fil::fixed_string {"struct:"}> {}
                     + fil::copa::match_identifier<fil::copa::member<&ast_object::data>> {}
                     + fil::copa::match_string<fil::fixed_string {"->"}> {}
                     + fil::copa::list_rule<fil::copa::match_parser<level2_grammar, fil::copa::member<&ast_object::nested>>> {}
                     + fil::copa::semicol;
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        struct deeply_nested_grammar {
            struct ast_object {
                level1_ast deep;
            };

            // match "{" + level1 + "};"
            static constexpr auto rules() {
                return fil::copa::bracket_wrapped< //
                           fil::copa::match_parser<level1_grammar, fil::copa::member<&ast_object::deep>>> {}
                     + fil::copa::semicol;
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        SECTION("One level1") {
            fil::buffer_reader reader(R"(
{
 struct:ffx ->  data: "chocobo";
};
)");
            deeply_nested_grammar grammar;
            const auto result = fil::copa::parse(grammar, std::move(reader));
            REQUIRE(result.has_value());
            CHECK(result->deep.data == "ffx");
            CHECK(result->deep.nested.size() == 1);
            CHECK(result->deep.nested[0].nested.data == "chocobo");
        }
        SECTION("Multiple level1") {
            fil::buffer_reader reader(R"(
{
 struct:ffx ->  data: "chocobo" data: "waka" data: "yuna";
};
)");
            deeply_nested_grammar grammar;
            const auto result = fil::copa::parse(grammar, std::move(reader));
            REQUIRE(result.has_value());
            CHECK(result->deep.data == "ffx");
            CHECK(result->deep.nested.size() == 3);
            CHECK(result->deep.nested[0].nested.data == "chocobo");
            CHECK(result->deep.nested[1].nested.data == "waka");
            CHECK(result->deep.nested[2].nested.data == "yuna");
        }
    }
}

TEST_CASE("Copa: repeat<N> rule", "[copa][repeat]") {
    SECTION("Fixed repetition") {
        // Test the repeat<N> rule to match exactly N occurrences.
        // Rule: repeat<3>(match_char<'X'>)
        // Input: "XXX" -> Success
        // Input: "XX" -> Failure
        struct repeat_ast {
            std::vector<char> v;
        }; // dummy
        struct repeat_grammar {
            using ast_object = repeat_ast;
            static constexpr auto rules() { return fil::copa::repeat<3>(fil::copa::match_char<'X', fil::copa::member<&ast_object::v>> {}); }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<repeat_ast> {}; }
        };

        fil::buffer_reader reader("XXX");
        repeat_grammar grammar;
        const auto result = fil::copa::parse(grammar, std::move(reader));
        REQUIRE(result.has_value());
        REQUIRE(result->v.size() == 3);
        CHECK(result->v[0] == 'X');
        CHECK(result->v[1] == 'X');
        CHECK(result->v[2] == 'X');
    }

    SECTION("Or repeat edge case") {
        struct times_edge_grammar {
            struct ast_object {
                std::vector<char> chars;
            };

            static constexpr auto rules() {
                return fil::copa::repeat<5>(fil::copa::match_char<'X', fil::copa::member<&ast_object::chars>> {}) // Three times
                     | fil::copa::repeat<3>(fil::copa::match_char<'X', fil::copa::member<&ast_object::chars>> {}) // Two times
                     // will never match because superior to 3 which will match before
                     | fil::copa::repeat<4>(fil::copa::match_char<'X', fil::copa::member<&ast_object::chars>> {}); // Five times
            }
            static constexpr auto convertor() { return fil::copa::sink::aggregator<ast_object> {}; }
        };

        SECTION("match 5") {
            // match first iteration
            fil::buffer_reader reader("XXXXX");
            times_edge_grammar grammar;
            const auto result = fil::copa::parse(grammar, std::move(reader));
            REQUIRE(result.has_value());
            REQUIRE(result->chars.size() == 5);
            CHECK(result->chars[0] == 'X');
            CHECK(result->chars[1] == 'X');
            CHECK(result->chars[2] == 'X');
            CHECK(result->chars[3] == 'X');
            CHECK(result->chars[4] == 'X');
        }

        SECTION("match 3") {
            // match first iteration
            fil::buffer_reader reader("XXX");
            times_edge_grammar grammar;
            const auto result = fil::copa::parse(grammar, std::move(reader));
            REQUIRE(result.has_value());
            REQUIRE(result->chars.size() == 3);
            CHECK(result->chars[0] == 'X');
            CHECK(result->chars[1] == 'X');
            CHECK(result->chars[2] == 'X');
        }

        SECTION("match 4 (actually match 3)") {
            // match first iteration
            fil::buffer_reader reader("XXXX");
            times_edge_grammar grammar;
            const auto result = fil::copa::parse(grammar, std::move(reader));
            REQUIRE(result.has_value());
            REQUIRE(result->chars.size() == 3);
            CHECK(result->chars[0] == 'X');
            CHECK(result->chars[1] == 'X');
            CHECK(result->chars[2] == 'X');
        }
    }
}

TEST_CASE("Copa: AST generator") {

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
    struct base_grammar {
        using ast_object = ast_node;

        static constexpr auto rules() { return fil::copa::match_number<ast_node::leaf> {}; }
        static constexpr auto convertor() { return fil::copa::sink::ast_tree_generator<ast_node> {0}; }
    };

    struct grammar {
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

    SECTION("parse simple : one node") {
        /* AST generated
                 +
               /   \
             1337   42
        */

        fil::buffer_reader reader("1337 + 42");

        grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());
        CHECK(result.value().value == op::plus);
        CHECK(std::holds_alternative<int>(result.value().lhs));
        CHECK(std::holds_alternative<int>(result.value().rhs));
        CHECK(std::get<int>(result.value().lhs) == 1337);
        CHECK(std::get<int>(result.value().rhs) == 42);
    }
    SECTION("parse simple : multiple node same/high precedence") {
        fil::buffer_reader reader("1337 + 42 - 10 + 2");
        /* AST generated
                +
              /   \
            1337    -
                  /   \
                42     +
                     /   \
                   10     2
         */

        grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());

        CHECK(result.value().value == op::plus);
        CHECK(std::holds_alternative<int>(result.value().lhs));
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().rhs));
        CHECK(std::get<int>(result.value().lhs) == 1337);

        auto node_rhs1 = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(node_rhs1 != nullptr);
        CHECK(node_rhs1->value == op::minus);
        CHECK(std::holds_alternative<int>(node_rhs1->lhs));
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(node_rhs1->rhs));
        CHECK(std::get<int>(node_rhs1->lhs) == 42);

        auto node_rhs2 = std::get<std::shared_ptr<ast_node>>(node_rhs1->rhs);
        REQUIRE(node_rhs2 != nullptr);
        CHECK(node_rhs2->value == op::plus);
        CHECK(std::holds_alternative<int>(node_rhs2->lhs));
        CHECK(std::holds_alternative<int>(node_rhs2->rhs));
        CHECK(std::get<int>(node_rhs2->lhs) == 10);
        CHECK(std::get<int>(node_rhs2->rhs) == 2);
    }

    SECTION("parse simple : two node low-precedence") {
        fil::buffer_reader reader("1337 * 42 + 10");
        /** AST generated
                 +
               /   \
              *      4
            /   \
          1337  42

        */
        grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());

        CHECK(result.value().value == op::plus);
        CHECK(std::holds_alternative<int>(result.value().rhs));
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().lhs));
        CHECK(std::get<int>(result.value().rhs) == 10);

        auto node_lhs1 = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(node_lhs1 != nullptr);
        CHECK(node_lhs1->value == op::multiply);
        CHECK(std::holds_alternative<int>(node_lhs1->lhs));
        CHECK(std::holds_alternative<int>(node_lhs1->rhs));
        CHECK(std::get<int>(node_lhs1->lhs) == 1337);
        CHECK(std::get<int>(node_lhs1->rhs) == 42);
    }
    SECTION("parse simple : two node higher-precedence") {
        fil::buffer_reader reader("1337 + 42 / 10");
        /** AST generated
                 +
               /   \
             1337   `/`
                   /   \
                 42    10

        */
        grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());

        CHECK(result.value().value == op::plus);
        CHECK(std::holds_alternative<int>(result.value().lhs));
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().rhs));
        CHECK(std::get<int>(result.value().lhs) == 1337);

        auto node_rhs1 = std::get<std::shared_ptr<ast_node>>(result.value().rhs);
        REQUIRE(node_rhs1 != nullptr);
        CHECK(node_rhs1->value == op::divide);
        CHECK(std::holds_alternative<int>(node_rhs1->lhs));
        CHECK(std::holds_alternative<int>(node_rhs1->rhs));
        CHECK(std::get<int>(node_rhs1->lhs) == 42);
        CHECK(std::get<int>(node_rhs1->rhs) == 10);
    }

    SECTION("parse simple : mixed node higher-lower-precedence") {
        fil::buffer_reader reader("1337 + 42 / 10 - 8");
        /** AST generated
                    -
                  /   \
                 +     8
               /   \
             1337   `/`
                   /   \
                 42    10

        */
        grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());

        CHECK(result.value().value == op::minus);
        CHECK(std::holds_alternative<int>(result.value().rhs));
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().lhs));
        CHECK(std::get<int>(result.value().rhs) == 8);

        auto node_lhs1 = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
        REQUIRE(node_lhs1 != nullptr);
        CHECK(node_lhs1->value == op::plus);
        CHECK(std::holds_alternative<int>(node_lhs1->lhs));
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(node_lhs1->rhs));
        CHECK(std::get<int>(node_lhs1->lhs) == 1337);

        auto node_rhs2 = std::get<std::shared_ptr<ast_node>>(node_lhs1->rhs);
        REQUIRE(node_rhs2 != nullptr);
        CHECK(node_rhs2->value == op::divide);
        CHECK(std::holds_alternative<int>(node_rhs2->lhs));
        CHECK(std::holds_alternative<int>(node_rhs2->rhs));
        CHECK(std::get<int>(node_rhs2->lhs) == 42);
        CHECK(std::get<int>(node_rhs2->rhs) == 10);
    }

    SECTION("parse more-complex : mixed node") {
        fil::buffer_reader reader("2 + 3 * 4 * 2 - 6 / 2 + 1");
        /** AST generated
                       +
                     /    \
                    -      1
                 /     \
                +       `/`
              /   \     /  \
             2     *   6    2
                 /   \
                3     *
                    /   \
                   4     2

        */
        grammar g;
        const auto result = fil::copa::parse(g, std::move(reader));
        REQUIRE(result.has_value());

        CHECK(result.value().value == op::plus);
        CHECK(std::holds_alternative<int>(result.value().rhs));
        CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(result.value().lhs));

        // right branch
        CHECK(std::get<int>(result.value().rhs) == 1);

        // check left branch
        {
            auto node_lhs1 = std::get<std::shared_ptr<ast_node>>(result.value().lhs);
            REQUIRE(node_lhs1 != nullptr);
            CHECK(node_lhs1->value == op::minus);
            CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(node_lhs1->lhs));
            CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(node_lhs1->rhs));

            {
                auto node_lhs2 = std::get<std::shared_ptr<ast_node>>(node_lhs1->lhs);
                REQUIRE(node_lhs2 != nullptr);
                CHECK(node_lhs2->value == op::plus);
                CHECK(std::holds_alternative<int>(node_lhs2->lhs));
                CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(node_lhs2->rhs));
                CHECK(std::get<int>(node_lhs2->lhs) == 2);

                auto node_lhs2_rhs = std::get<std::shared_ptr<ast_node>>(node_lhs2->rhs);
                CHECK(node_lhs2_rhs->value == op::multiply);
                CHECK(std::holds_alternative<int>(node_lhs2_rhs->lhs));
                CHECK(std::holds_alternative<std::shared_ptr<ast_node>>(node_lhs2_rhs->rhs));
                CHECK(std::get<int>(node_lhs2_rhs->lhs) == 3);

                auto node_lhs2_rhs_rhs = std::get<std::shared_ptr<ast_node>>(node_lhs2_rhs->rhs);
                CHECK(node_lhs2_rhs_rhs->value == op::multiply);
                CHECK(std::holds_alternative<int>(node_lhs2_rhs_rhs->lhs));
                CHECK(std::holds_alternative<int>(node_lhs2_rhs_rhs->rhs));
                CHECK(std::get<int>(node_lhs2_rhs_rhs->lhs) == 4);
                CHECK(std::get<int>(node_lhs2_rhs_rhs->rhs) == 2);
            }
            {
                auto node_rhs2 = std::get<std::shared_ptr<ast_node>>(node_lhs1->rhs);

                REQUIRE(node_rhs2 != nullptr);
                CHECK(node_rhs2->value == op::divide);
                CHECK(std::holds_alternative<int>(node_rhs2->lhs));
                CHECK(std::holds_alternative<int>(node_rhs2->rhs));
                CHECK(std::get<int>(node_rhs2->lhs) == 6);
                CHECK(std::get<int>(node_rhs2->rhs) == 2);
            }
        }
    }

    SECTION("parse : sub graph") {
        fil::buffer_reader reader("2 * (3 + 4)");

        /* sub graph is the addition -- added as a leaf to the multiplication
         *
               /   \
              2      +
                   /   \
                  3     4
        */
    }
}

} // namespace
