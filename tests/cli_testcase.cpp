// MIT License
//
// Copyright (c) 2020 Quentin Balland
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

#include <catch2/catch.hpp>
#include <fil/cli/command_line_interface.hh>
#include <iostream>

TEST_CASE("cli_test_case Simple", "[cli]") {
   bool action_base_has_been_called = false;
   fil::command_line_interface cli(
	   [&action_base_has_been_called]() { action_base_has_been_called = true; },
	   "A Simple Command Line tool");

   std::vector<std::string> opt_arg_called;
   const auto& call_handler_with_arg = cli.add_option(fil::option(
	   "--opt-with-arg",
	   [&opt_arg_called](std::string arg) { opt_arg_called.emplace_back(std::move(arg)); },
	   "command with arg"));

   bool opt_no_arg_called = false;
   const auto& call_handler_without_arg = cli.add_option(fil::option(
	   "--opt-no-arg",
	   [&opt_no_arg_called]() { opt_no_arg_called = true; },
	   "command without arg"));

   std::vector<std::string> cli_argument;
   cli.on_parameter_handler([&cli_argument](std::string param) { cli_argument.emplace_back(std::move(param)); });

   CHECK_FALSE(call_handler_without_arg);
   CHECK_FALSE(call_handler_with_arg);

   SECTION("called alone") {
	  char* args[] = {"cli"};
	  cli.parse_command_line(1, args);

	  CHECK(action_base_has_been_called);
   }

   SECTION("with one option") {
	  char* args[] = {"cli", "--opt-no-arg"};
	  cli.parse_command_line(2, args);

	  CHECK(action_base_has_been_called);
	  CHECK(opt_no_arg_called);
	  CHECK(call_handler_without_arg);
   }

   SECTION("with one option with argument") {
	  char* args[] = {"cli", "--opt-with-arg", "this_is_an_argument"};
	  cli.parse_command_line(3, args);

	  CHECK(action_base_has_been_called);
	  REQUIRE(1 == opt_arg_called.size());
	  CHECK("this_is_an_argument" == opt_arg_called.at(0));
	  CHECK(call_handler_with_arg);
   }

   SECTION("with options") {
	  char* args[] = {
		  "cli",
		  "--opt-with-arg", "this_is_an_argument",
		  "--opt-with-arg", "option_arg_2",
		  "--opt-with-arg", "option_arg_3",
		  "--opt-no-arg"};

	  cli.parse_command_line(8, args);

	  CHECK(action_base_has_been_called);
	  CHECK(opt_no_arg_called);
	  REQUIRE(3 == opt_arg_called.size());
	  CHECK("this_is_an_argument" == opt_arg_called.at(0));
	  CHECK("option_arg_2" == opt_arg_called.at(1));
	  CHECK("option_arg_3" == opt_arg_called.at(2));
   }

   SECTION("with one argument") {
	  char* args[] = {"cli", "cli_argument_1"};
	  cli.parse_command_line(2, args);

	  CHECK(action_base_has_been_called);
	  REQUIRE(1 == cli_argument.size());
	  CHECK("cli_argument_1" == cli_argument.at(0));
   }

   SECTION("with arguments") {
	  char* args[] = {"cli", "cli_argument_1", "cli_argument_2", "cli_argument_3"};
	  cli.parse_command_line(4, args);

	  CHECK(action_base_has_been_called);
	  REQUIRE(3 == cli_argument.size());
	  CHECK("cli_argument_1" == cli_argument.at(0));
	  CHECK("cli_argument_2" == cli_argument.at(1));
	  CHECK("cli_argument_3" == cli_argument.at(2));
   }

   SECTION("check --help exist") {
	  char* args[] = {"cli", "--help"};
	  CHECK_NOTHROW([&]() { cli.parse_command_line(2, args); });
   }

   SECTION("failure on unknown option") {
	  char* args[] = {"cli", "--unknown-opt"};
	  CHECK_THROWS_AS([&]() { cli.parse_command_line(2, args); }(), std::invalid_argument);
   }

}// End TestCase : cli_test_case Simple

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
	  char* args[] = {"cli", "sub_com_1"};
	  CHECK_THROWS(cli.parse_command_line(2, args));
   }

   SECTION("sub_com1 inner_inner") {
	  char* args[] = {"cli", "sub_com_1", "inner_inner"};
	  CHECK_NOTHROW(cli.parse_command_line(3, args));
	  CHECK(call_inner_inner);
   }

   SECTION("sub_com1 opt-inner-inner") {
	  char* args[] = {"cli", "sub_com_1", "inner_inner", "--opt-inner-inner"};
	  CHECK_NOTHROW(cli.parse_command_line(4, args));
	  CHECK(call_inner_inner);
	  CHECK(call_option_inner_inner);
   }

   SECTION("sub_com2") {
	  char* args[] = {"cli", "sub_com_2"};
	  CHECK_NOTHROW(cli.parse_command_line(2, args));
	  CHECK(call_sub2);
   }

   SECTION("sub_com2 option") {
	  char* args[] = {"cli", "sub_com_2", "--opt-sub"};
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
   cli.add_option(fil::option(
	   "--opt-no-arg",
	   [&opt_no_arg_called_SHOULD_NOT_BE_CALLED]() {
		  opt_no_arg_called_SHOULD_NOT_BE_CALLED = true;
	   },
	   "command without arg"));

   bool opt_no_arg_called = false;
   sub.add_option(fil::option(
	   "--opt-no-arg",
	   [&opt_no_arg_called]() { opt_no_arg_called = true; },
	   "command without arg"));

   std::vector<std::string> opt_arg_called;
   sub.add_option(fil::option(
	   "--opt-with-arg",
	   [&opt_arg_called](std::string arg) { opt_arg_called.emplace_back(std::move(arg)); },
	   "command with arg"));

   std::vector<std::int64_t> opt_int_called;
   sub.add_option(fil::option(
	   "--opt-with-int",
	   [&opt_int_called](std::int64_t arg) { opt_int_called.emplace_back(arg); },
	   "command with integer arg"));

   std::vector<std::string> sub_argument;
   sub.on_parameter_handler([&sub_argument](std::string param) { sub_argument.emplace_back(std::move(param)); });

   cli.add_sub_command(sub);

   std::vector<std::string> cli_argument;
   cli.on_parameter_handler([&cli_argument](std::string param) { cli_argument.emplace_back(std::move(param)); });

   SECTION("help cli") {
	  char* args[] = {"cli", "--help"};
	  CHECK(cli.parse_command_line(2, args));
   }

   SECTION("help sub") {
	  char* args[] = {"cli", "command_of_doom", "--help"};
	  CHECK(cli.parse_command_line(3, args));

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
   }

   SECTION("called alone") {
	  char* args[] = {"cli", "command_of_doom"};
	  cli.parse_command_line(2, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
   }

   SECTION("one option") {
	  char* args[] = {"cli", "command_of_doom", "--opt-no-arg"};
	  cli.parse_command_line(3, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK_FALSE(opt_no_arg_called_SHOULD_NOT_BE_CALLED);
	  CHECK(action_sub_has_been_called);
	  CHECK(opt_no_arg_called);
   }

   SECTION("with one option with arg") {
	  char* args[] = {"cli", "command_of_doom", "--opt-with-arg", "this_is_an_argument"};
	  cli.parse_command_line(4, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
	  REQUIRE(1 == opt_arg_called.size());
	  CHECK("this_is_an_argument" == opt_arg_called.at(0));
   }

   SECTION("with one option with int arg") {
	  char* args[] = {"cli", "command_of_doom", "--opt-with-int", "42", "--opt-with-int", "1337"};
	  cli.parse_command_line(6, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
	  REQUIRE(2 == opt_int_called.size());
	  CHECK(42 == opt_int_called.at(0));
	  CHECK(1337 == opt_int_called.at(1));
   }

   SECTION("with options") {
	  char* args[] = {
		  "cli", "command_of_doom",
		  "--opt-with-arg", "this_is_an_argument",
		  "--opt-with-arg", "option_arg_2",
		  "--opt-with-arg", "option_arg_3",
		  "--opt-no-arg"};

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
	  char* args[] = {"cli", "command_of_doom", "cli_argument_1"};
	  cli.parse_command_line(3, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
	  REQUIRE(1 == sub_argument.size());
	  CHECK("cli_argument_1" == sub_argument.at(0));
   }

   SECTION("with arguments") {
	  char* args[] = {"cli", "command_of_doom", "cli_argument_1", "cli_argument_2", "cli_argument_3"};
	  cli.parse_command_line(5, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
	  REQUIRE(3 == sub_argument.size());
	  CHECK("cli_argument_1" == sub_argument.at(0));
	  CHECK("cli_argument_2" == sub_argument.at(1));
	  CHECK("cli_argument_3" == sub_argument.at(2));
   }

   SECTION("failure on chaining arguments then commands") {
	  char* args[] = {"cli", "cli_argument_1", "cli_argument_2", "cli_argument_3", "command_of_doom"};
	  CHECK_THROWS_AS([&]() { cli.parse_command_line(5, args); }(), std::invalid_argument);
   }

   SECTION("failure on unknown subcommand option") {
	  char* args[] = {"cli", "command_of_doom", "--unknown-opt"};
	  CHECK_THROWS_AS([&]() { cli.parse_command_line(3, args); }(), std::invalid_argument);
   }

   SECTION("check subcommand --help exist") {
	  char* args[] = {"cli", "command_of_doom", "--help"};
	  CHECK_NOTHROW([&]() { cli.parse_command_line(3, args); });
   }

}// End TestCase : cli_test_case SubCommand One Layer

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
   fil::cli::add_argument_option(sub, "--oo", arg_opt);

   int arg_int_opt;
   fil::cli::add_argument_option(sub, "--ooint", arg_int_opt);

   std::uint64_t arg_uint_opt;
   fil::cli::add_argument_option(sub, "--oouint", arg_uint_opt);

   std::vector<std::string> vec_arg;
   fil::cli::add_multi_arg(sub, vec_arg);

   cli.add_sub_command(std::move(sub));
   cli.set_sub_command_only(true);

   SECTION("add_argument_option") {
	  char* args[] = {"cli", "command_of_doom", "--oo", "argumentation_of_doom"};
	  cli.parse_command_line(4, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
	  CHECK("argumentation_of_doom" == arg_opt);
   }// End section :

   SECTION("add_argument_option int") {
	  char* args[] = {"cli", "command_of_doom", "--ooint", "-42"};
	  cli.parse_command_line(4, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
	  CHECK(-42 == arg_int_opt);
   }// End section :

   SECTION("add_argument_option uint") {
	  char* args[] = {"cli", "command_of_doom", "--oouint", "42"};
	  cli.parse_command_line(4, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
	  CHECK(42 == arg_uint_opt);
   }// End section :

   SECTION("add_multi_arg") {
	  char* args[] = {"cli", "command_of_doom", "argumentation_of_doom1", "argumentation_of_doom2"};
	  cli.parse_command_line(4, args);

	  CHECK_FALSE(action_base_has_been_called);
	  CHECK(action_sub_has_been_called);
	  CHECK(2 == vec_arg.size());
	  CHECK("argumentation_of_doom1" == vec_arg[0]);
	  CHECK("argumentation_of_doom2" == vec_arg[1]);
   }// End section : add_multi_arg

   SECTION("error on sub_commmand_only") {
	  char* args[] = {"cli"};

	  CHECK_THROWS_AS([&]() { cli.parse_command_line(1, args); }(), std::invalid_argument);
   }// End section :
}
