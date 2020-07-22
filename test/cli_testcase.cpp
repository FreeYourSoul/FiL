// MIT License
//
// Copyright (c) 2019 Quentin Balland
// Repository : https://github.com/FreeYourSoul/FyS
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

#include <cli/command_line_interface.hh>
#include <catch2/catch.hpp>
#include <iostream>

TEST_CASE("cli_test_case Simple", "[cli]")
{
	bool action_base_has_been_called = false;
	fil::command_line_interface cli(
			[&action_base_has_been_called]() { action_base_has_been_called = true; },
			"A Simple Command Line tool");

	bool opt_no_arg_called = false;
	cli.add_option(fil::option("--opt-no-arg",
			[&opt_no_arg_called]() { opt_no_arg_called = true; },
			"command without arg"));

	std::vector<std::string> opt_arg_called;
	cli.add_option(fil::option("--opt-with-arg",
			[&opt_arg_called](std::string arg) { opt_arg_called.emplace_back(std::move(arg)); },
			"command with arg"));

	std::vector<std::string> cli_argument;
	cli.on_parameter_handler([&cli_argument](std::string param) { cli_argument.emplace_back(std::move(param)); });

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
	}

	SECTION("with one option with argument") {
		char* args[] = {"cli", "--opt-with-arg", "this_is_an_argument"};
		cli.parse_command_line(3, args);

		CHECK(action_base_has_been_called);
		REQUIRE(1 == opt_arg_called.size());
		CHECK("this_is_an_argument" == opt_arg_called.at(0));
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

	}

	SECTION("with arguments") { }

	SECTION("check --help exist") { }

	SECTION("failure on unknown option") {
		char* args[] = {"cli", "--unknown-opt"};
		CHECK_THROWS_AS([&]() { cli.parse_command_line(2, args); }(), std::invalid_argument);
	}

} // End TestCase : cli_test_case Simple


TEST_CASE("cli_test_case SubCommand One Layer", "[cli]")
{
	SECTION("one option") { }

	SECTION("with options") { }

	SECTION("with one argument") { }

	SECTION("with arguments") { }

	SECTION("failure on unknown subcommand") { }

	SECTION("failure on chaining arguments then commands") { }

	SECTION("failure on unknown subcommand option") { }

	SECTION("check subcommand --help exist") { }

} // End TestCase : cli_test_case SubCommand One Layer


TEST_CASE("cli_test_case Deep Layering", "[cli]")
{

} // End TestCase : cli_test_case SubCommand One Layer
