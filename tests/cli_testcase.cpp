// MIT License
//
// Copyright (c) 2025 Quentin Balland
// Repository : https://github.com/FreeYourSoul/FiL
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//         of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
//         to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//         copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
//         copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//         AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <catch2/catch_test_macros.hpp>
#include <fil/cli/command_line_interface.hh>
#include <iostream>

TEST_CASE("cli_test_case Simple", "[cli]") {
    bool action_base_has_been_called = false;
    fil::command_line_interface cli([&action_base_has_been_called]() { action_base_has_been_called = true; }, "A Simple Command Line tool");

    std::vector<std::string> opt_arg_called;
    const auto& call_handler_with_arg = cli.add_option(fil::option(
        "--opt-with-arg", [&opt_arg_called](std::string arg) { opt_arg_called.emplace_back(std::move(arg)); }, "command with arg"));

    bool opt_no_arg_called = false;
    const auto& call_handler_without_arg =
        cli.add_option(fil::option("--opt-no-arg", [&opt_no_arg_called]() { opt_no_arg_called = true; }, "command without arg"));

    std::vector<std::string> cli_argument;
    cli.on_parameter_handler([&cli_argument](std::string param) { cli_argument.emplace_back(std::move(param)); });

    CHECK_FALSE(call_handler_without_arg);
    CHECK_FALSE(call_handler_with_arg);

    SECTION("called alone") {
        char* args[] = {const_cast<char*>("cli")};
        cli.parse_command_line(1, args);

        CHECK(action_base_has_been_called);
    }

    SECTION("with one option") {
        char* args[] = {const_cast<char*>("cli"), const_cast<char*>("--opt-no-arg")};
        cli.parse_command_line(2, args);

        CHECK(action_base_has_been_called);
        CHECK(opt_no_arg_called);
        CHECK(call_handler_without_arg);
    }

    SECTION("with one option with argument") {
        char* args[] = {const_cast<char*>("cli"), const_cast<char*>("--opt-with-arg"), const_cast<char*>("this_is_an_argument")};
        cli.parse_command_line(3, args);

        CHECK(action_base_has_been_called);
        REQUIRE(1 == opt_arg_called.size());
        CHECK("this_is_an_argument" == opt_arg_called.at(0));
        CHECK(call_handler_with_arg);
    }

    SECTION("with options") {
        char* args[] = {const_cast<char*>("cli"),
                        const_cast<char*>("--opt-with-arg"),
                        const_cast<char*>("this_is_an_argument"),
                        const_cast<char*>("--opt-with-arg"),
                        const_cast<char*>("option_arg_2"),
                        const_cast<char*>("--opt-with-arg"),
                        const_cast<char*>("option_arg_3"),
                        const_cast<char*>("--opt-no-arg")};

        cli.parse_command_line(8, args);

        CHECK(action_base_has_been_called);
        CHECK(opt_no_arg_called);
        REQUIRE(3 == opt_arg_called.size());
        CHECK("this_is_an_argument" == opt_arg_called.at(0));
        CHECK("option_arg_2" == opt_arg_called.at(1));
        CHECK("option_arg_3" == opt_arg_called.at(2));
    }

    SECTION("with one argument") {
        char* args[] = {const_cast<char*>("cli"), const_cast<char*>("cli_argument_1")};
        cli.parse_command_line(2, args);

        CHECK(action_base_has_been_called);
        REQUIRE(1 == cli_argument.size());
        CHECK("cli_argument_1" == cli_argument.at(0));
    }

    SECTION("with arguments") {
        char* args[] = {const_cast<char*>("cli"), const_cast<char*>("cli_argument_1"), const_cast<char*>("cli_argument_2"),
                        const_cast<char*>("cli_argument_3")};
        cli.parse_command_line(4, args);

        CHECK(action_base_has_been_called);
        REQUIRE(3 == cli_argument.size());
        CHECK("cli_argument_1" == cli_argument.at(0));
        CHECK("cli_argument_2" == cli_argument.at(1));
        CHECK("cli_argument_3" == cli_argument.at(2));
    }

    SECTION("check --help exist") {
        char* args[] = {const_cast<char*>("cli"), const_cast<char*>("--help")};
        CHECK_NOTHROW([&]() { cli.parse_command_line(2, args); });
    }

    SECTION("failure on unknown option") {
        char* args[] = {const_cast<char*>("cli"), const_cast<char*>("--unknown-opt")};
        CHECK_THROWS_AS([&]() { cli.parse_command_line(2, args); }(), std::invalid_argument);
    }

} // End TestCase : cli_test_case Simple

TEST_CASE("cli_test_case OneLiner Constructor", "[cli]") {

    bool call_inner_inner {}, call_sub2 {}, call_option_sub {}, call_option_inner_inner {};

    // clang-format off
   fil::command_line_interface cli({

	  fil::sub_command("sub_com_1", "The fist subcommand", {

		fil::sub_command("inner_inner", [&call_inner_inner]() { call_inner_inner = true; }, "inception of command option", {}, {
		   fil::option("--opt-inner-inner", [&call_option_inner_inner]() { call_option_inner_inner = true; }, "inner inner option")
		 })

	  }),

	  fil::sub_command("sub_com_2", [&call_sub2]() { call_sub2 = true; }, "The fist subcommand", {}, {
	  	fil::option("--opt-sub", [&call_option_sub]() { call_option_sub = true; }, "sub option")
	  })

	},
	{}, "A Simple Command line tool");
   // clang-format off

   CHECK_FALSE(call_inner_inner);
   CHECK_FALSE(call_option_inner_inner);
   CHECK_FALSE(call_option_sub);
   CHECK_FALSE(call_sub2);

   SECTION("sub_com1 only subcommand allowed") {
	  char* args[] = {const_cast<char*>("cli"), const_cast<char*>("sub_com_1")};
	  CHECK_THROWS(cli.parse_command_line(2, args));
   }

   SECTION("sub_com1 inner_inner") {
	  char* args[] = {const_cast<char*>("cli"), const_cast<char*>("sub_com_1"), const_cast<char*>("inner_inner")};
	  CHECK_NOTHROW(cli.parse_command_line(3, args));
	  CHECK(call_inner_inner);
   }

   SECTION("sub_com1 opt-inner-inner") {
	  char* args[] = {const_cast<char*>("cli"), const_cast<char*>("sub_com_1"), const_cast<char*>("inner_inner"), const_cast<char*>("--opt-inner-inner")};
	  CHECK_NOTHROW(cli.parse_command_line(4, args));
	  CHECK(call_inner_inner);
	  CHECK(call_option_inner_inner);
   }

   SECTION("sub_com2") {
	  char* args[] = {const_cast<char*>("cli"), const_cast<char*>("sub_com_2")};
	  CHECK_NOTHROW(cli.parse_command_line(2, args));
	  CHECK(call_sub2);
   }

   SECTION("sub_com2 option") {
	  char* args[] = {const_cast<char*>("cli"), const_cast<char*>("sub_com_2"), const_cast<char*>("--opt-sub")};
	  CHECK_NOTHROW(cli.parse_command_line(3, args));
	  CHECK(call_option_sub);
   }

}// End TestCase : Test OneLiner Constructor

TEST_CASE("cli_test_case SubCommand One Layer", "[cli]") {

   bool action_base_has_been_called = false;
   fil::command_line_interface cli(
	   [&action_base_has_been_called]() { action_base_has_been_called = true; },
	   "A Simple Command Line tool");

   bool action_sub_has_been_called = false;
   auto sub = fil::sub_command(
	   "command_of_doom",
	   [&action_sub_has_been_called] { action_sub_has_been_called = true; },
	   "Sub Command Helper doc");

   bool opt_no_arg_called_SHOULD_NOT_BE_CALLED = false;
    {
       [[maybe_unused]]const auto _ = cli.add_option(fil::option(
           "--opt-no-arg",
           [&opt_no_arg_called_SHOULD_NOT_BE_CALLED]() {
              opt_no_arg_called_SHOULD_NOT_BE_CALLED = true;
           },
           "command without arg"));
    }
    std::vector<std::string> cli_opt_arg_called;
    {
       [[maybe_unused]]const auto _ = cli.add_option(fil::option(
          "--opt-with-arg",
          [&cli_opt_arg_called](std::string arg) { cli_opt_arg_called.emplace_back(std::move(arg)); },
          "command with arg for cli"));
    }

   bool opt_no_arg_called = false;
    {
        [[maybe_unused]]const auto _ = sub.add_option(fil::option(
           "--opt-no-arg",
           [&opt_no_arg_called]() { opt_no_arg_called = true; },
           "command without arg"));
    }

   std::vector<std::string> opt_arg_called;
    {
        [[maybe_unused]]const auto _ = sub.add_option(fil::option(
           "--opt-with-arg",
           [&opt_arg_called](std::string arg) { opt_arg_called.emplace_back(std::move(arg)); },
           "command with arg"));
    }

   std::vector<std::int64_t> opt_int_called;
    {
        [[maybe_unused]]const auto _ = sub.add_option(fil::option(
           "--opt-with-int",
           [&opt_int_called](std::int64_t arg) { opt_int_called.emplace_back(arg); },
           "command with integer arg"));
    }

   std::vector<std::string> sub_argument;
   sub.on_parameter_handler([&sub_argument](std::string param) { sub_argument.emplace_back(std::move(param)); });

   [[maybe_unused]]const auto _ = cli.add_sub_command(sub);

   std::vector<std::string> cli_argument;
   cli.on_parameter_handler([&cli_argument](std::string param) { cli_argument.emplace_back(std::move(param)); });

   SECTION("help cli") {
	  char* args[] = {const_cast<char*>("cli"), const_cast<char*>("--help")};
	  CHECK(cli.parse_command_line(2, args));
   }

    SECTION("help sub") {
       char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom"), const_cast<char*>("--help")};
       CHECK(cli.parse_command_line(3, args));

       CHECK_FALSE(action_base_has_been_called);
       CHECK(action_sub_has_been_called);
   }

    SECTION("main command option works with subcommand") {
       char* args[] = {const_cast<char*>("cli"), const_cast<char*>("--opt-no-arg"), const_cast<char*>("command_of_doom"), const_cast<char*>("--help")};
       CHECK(cli.parse_command_line(4, args));

       CHECK(opt_no_arg_called_SHOULD_NOT_BE_CALLED);
       CHECK_FALSE(action_base_has_been_called);
       CHECK(action_sub_has_been_called);
   }

    SECTION("main command option with arg works with subcommand") {
       char* args[] = {const_cast<char*>("cli"), const_cast<char*>("--opt-with-arg"), const_cast<char*>("dada"), const_cast<char*>("command_of_doom"), const_cast<char*>("--help")};
       CHECK(cli.parse_command_line(5, args));

       CHECK(!cli_opt_arg_called.empty());
       CHECK(cli_opt_arg_called.at(0) == "dada");
       CHECK_FALSE(action_base_has_been_called);
       CHECK(action_sub_has_been_called);
   }

   SECTION("called alone") {
	  char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom")};
	  cli.parse_command_line(2, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
   }

   SECTION("one option") {
	  char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom"), const_cast<char*>("--opt-no-arg")};
	  cli.parse_command_line(3, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK_FALSE(opt_no_arg_called_SHOULD_NOT_BE_CALLED);
	  CHECK(action_sub_has_been_called);
	  CHECK(opt_no_arg_called);
   }

   SECTION("with one option with arg") {
	  char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom"), const_cast<char*>("--opt-with-arg"), const_cast<char*>("this_is_an_argument")};
	  cli.parse_command_line(4, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
	  REQUIRE(1 == opt_arg_called.size());
	  CHECK("this_is_an_argument" == opt_arg_called.at(0));
   }

   SECTION("with one option with int arg") {
	  char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom"), const_cast<char*>("--opt-with-int"), const_cast<char*>("42"), const_cast<char*>("--opt-with-int"), const_cast<char*>("1337")};
	  cli.parse_command_line(6, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
	  REQUIRE(2 == opt_int_called.size());
	  CHECK(42 == opt_int_called.at(0));
	  CHECK(1337 == opt_int_called.at(1));
   }

   SECTION("with options") {
	  char* args[] = {
		  const_cast<char*>("cli"), const_cast<char*>("command_of_doom"),
		  const_cast<char*>("--opt-with-arg"), const_cast<char*>("this_is_an_argument"),
		  const_cast<char*>("--opt-with-arg"), const_cast<char*>("option_arg_2"),
		  const_cast<char*>("--opt-with-arg"), const_cast<char*>("option_arg_3"),
		  const_cast<char*>("--opt-no-arg")};

	  cli.parse_command_line(9, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
	  CHECK(opt_no_arg_called);
	  REQUIRE(3 == opt_arg_called.size());
	  CHECK("this_is_an_argument" == opt_arg_called.at(0));
	  CHECK("option_arg_2" == opt_arg_called.at(1));
	  CHECK("option_arg_3" == opt_arg_called.at(2));
   }

   SECTION("with one argument") {
	  char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom"), const_cast<char*>("cli_argument_1")};
	  cli.parse_command_line(3, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
	  REQUIRE(1 == sub_argument.size());
	  CHECK("cli_argument_1" == sub_argument.at(0));
   }

   SECTION("with arguments") {
	  char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom"), const_cast<char*>("cli_argument_1"), const_cast<char*>("cli_argument_2"), const_cast<char*>("cli_argument_3")};
	  cli.parse_command_line(5, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
	  REQUIRE(3 == sub_argument.size());
	  CHECK("cli_argument_1" == sub_argument.at(0));
	  CHECK("cli_argument_2" == sub_argument.at(1));
	  CHECK("cli_argument_3" == sub_argument.at(2));
   }

   SECTION("failure on unknown subcommand option") {
	  char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom"), const_cast<char*>("--unknown-opt")};
	  CHECK_THROWS_AS([&]() { cli.parse_command_line(3, args); }(), std::invalid_argument);
   }

   SECTION("check subcommand --help exist") {
	  char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom"), const_cast<char*>("--help")};
	  CHECK_NOTHROW([&]() { cli.parse_command_line(3, args); });
   }

}// End TestCase : cli_test_case SubCommand One Layer

TEST_CASE("cli_test_case PreExecution", "[cli]") {
    // test that a cli pre-execution handler is called before the command is executed

    bool main_command_has_been_called = false;
    std::size_t calling_order = 0;
    fil::command_line_interface cli(
        [&]{ main_command_has_been_called = true; ++calling_order; },
        "A Simple Command Line tool");

    bool pre_check_called = false;
    cli.add_pre_executed_handler([&]{ pre_check_called = true; ++calling_order; });

    bool action_sub_has_been_called = false;
    auto sub = fil::sub_command(
        "command_of_doom",
        [&] { action_sub_has_been_called = true; ++calling_order; },
        "Sub Command Helper doc");

    bool action_sub_inner_has_been_called = false;
    auto sub2 = fil::sub_command(
        "command_of_doom_inner",
        [&] { action_sub_inner_has_been_called = true; ++calling_order; },
        "Sub Command Helper doc");

    [[maybe_unused]] const auto x = sub.add_sub_command(sub2);
    [[maybe_unused]] const auto y = cli.add_sub_command(sub);

    SECTION("normal pre-execution is called on normal execution of the cli") {
        char* args[] = {const_cast<char*>("cli")};
        cli.parse_command_line(1, args);

        CHECK(calling_order == 2); // 2 calling order meaning that the pre-execution handler has been called before the main action
        CHECK(pre_check_called); // confirm
        CHECK(main_command_has_been_called);
    }

    SECTION("check that the execution on a sub-command would result in the execution before it") {
        char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom")};
        cli.parse_command_line(2, args);

        CHECK(calling_order == 2); // 2 calling, the sub-command and he pre-execution handler
        CHECK(pre_check_called); // confirm
        CHECK(action_sub_has_been_called);
        CHECK_FALSE(main_command_has_been_called); // main command should not have been called
    }

    SECTION("check that two layer of sub-command makes the execution of the pre-command only once") {
        char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom"), const_cast<char*>("command_of_doom_inner")};
        cli.parse_command_line(3, args);

        CHECK(calling_order == 2); // 2 calling, the sub-sub-command and the pre-execution handler
        CHECK(pre_check_called); // confirm
        CHECK(action_sub_inner_has_been_called);
        CHECK_FALSE(action_sub_has_been_called);
        CHECK_FALSE(main_command_has_been_called); // main command should not have been called
    }

}

TEST_CASE("cli_test_case utility", "[cli]") {
   bool action_base_has_been_called = false;
   fil::command_line_interface cli(
	   [&action_base_has_been_called]() { action_base_has_been_called = true; },
	   "A Simple Command Line tool");

   bool action_sub_has_been_called = false;
   auto sub = fil::sub_command(
	   "command_of_doom",
	   [&action_sub_has_been_called]() { action_sub_has_been_called = true; },
	   "Sub Command Helper doc");

   std::string arg_opt;
   fil::cli::add_aliased_argument_option(sub, "--oo", "-x", arg_opt);

   int arg_int_opt;
   fil::cli::add_aliased_argument_option(sub, "--ooint", "-i", arg_int_opt);

   std::uint64_t arg_uint_opt;
   fil::cli::add_argument_option(sub, "--oouint", arg_uint_opt);

   std::vector<std::string> vec_arg;
   fil::cli::add_multi_arg(sub, vec_arg);

   [[maybe_unused]]const auto _ = cli.add_sub_command(sub);
   cli.set_sub_command_only(true);

    SECTION("add_argument_option") {
        char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom"), const_cast<char*>("--oo"), const_cast<char*>("argumentation_of_doom")};
        cli.parse_command_line(4, args);

        CHECK_FALSE(action_base_has_been_called);
        CHECK(action_sub_has_been_called);
        CHECK("argumentation_of_doom" == arg_opt);
    }// End section :

    SECTION("add_argument_option_alias") {
        char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom"), const_cast<char*>("-x"), const_cast<char*>("argumentation_of_doom")};
        cli.parse_command_line(4, args);

        CHECK_FALSE(action_base_has_been_called);
        CHECK(action_sub_has_been_called);
        CHECK("argumentation_of_doom" == arg_opt);
    }// End section :

    SECTION("add_argument_option int") {
        char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom"), const_cast<char*>("--ooint"), const_cast<char*>("-42")};
        cli.parse_command_line(4, args);

        CHECK_FALSE(action_base_has_been_called);
        CHECK(action_sub_has_been_called);
        CHECK(-42 == arg_int_opt);
    }// End section :

    SECTION("add_argument_option int alias") {
        char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom"), const_cast<char*>("-i"), const_cast<char*>("-42")};
        cli.parse_command_line(4, args);

        CHECK_FALSE(action_base_has_been_called);
        CHECK(action_sub_has_been_called);
        CHECK(-42 == arg_int_opt);
    }// End section :

   SECTION("add_argument_option uint") {
	  char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom"), const_cast<char*>("--oouint"), const_cast<char*>("42")};
	  cli.parse_command_line(4, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
	  CHECK(42 == arg_uint_opt);
   }// End section :

   SECTION("add_multi_arg") {
	  char* args[] = {const_cast<char*>("cli"), const_cast<char*>("command_of_doom"), const_cast<char*>("argumentation_of_doom1"), const_cast<char*>("argumentation_of_doom2")};
	  cli.parse_command_line(4, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
	  CHECK(2 == vec_arg.size());
	  CHECK("argumentation_of_doom1" == vec_arg[0]);
	  CHECK("argumentation_of_doom2" == vec_arg[1]);
   }// End section : add_multi_arg

   SECTION("error on sub_commmand_only") {
	  char* args[] = {const_cast<char*>("cli")};

	  CHECK_THROWS_AS([&]() { cli.parse_command_line(1, args); }(), std::invalid_argument);
   }// End section :
}
