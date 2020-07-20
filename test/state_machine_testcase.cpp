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

#include <fsm/state_machine.hh>
#include <catch2/catch.hpp>
#include <iostream>

TEST_CASE("state_machine_test_case", "[fsm]")
{
	enum StateTest : std::uint32_t {
		STATE_1,
		STATE_2,
		STATE_3,
		STATE_4,
		STATE_5
	};

	SECTION("Go through") {
		fil::state_machine<StateTest> s(STATE_1);
		bool check_transition_1 = false;
		bool check_transition_2 = false;
		bool check_transition_3 = false;
		bool check_transition_4 = false;

		s.add_transition(STATE_1, STATE_2, [&check_transition_1](){ check_transition_1 = true; return true;});
		s.add_transition(STATE_2, STATE_3, [&check_transition_2](){ check_transition_2 = true; return true;});
		s.add_transition(STATE_3, STATE_4, [&check_transition_3](){ check_transition_3 = true; return true;});
		s.add_transition(STATE_4, STATE_5, [&check_transition_4](){ check_transition_4 = true; return true;});

		auto o = s.advance();

		CHECK(check_transition_1);
		CHECK(check_transition_2);
		CHECK(check_transition_3);
		CHECK(check_transition_4);
		CHECK(STATE_5 == o);
	} // End section : Go through

	SECTION("Pass only once") {
		fil::state_machine<StateTest> s(STATE_1);
		int pass_1 = 0;
		int pass_2 = 0;
		int pass_3 = 0;
		int pass_4 = 0;

		s.add_transition(STATE_1, STATE_2, [&pass_1](){ ++pass_1; return true;});
		s.add_transition(STATE_2, STATE_3, [&pass_2](){ ++pass_2; return true;});
		s.add_transition(STATE_3, STATE_4, [&pass_3](){ ++pass_3; return true;});
		s.add_transition(STATE_4, STATE_5, [&pass_4](){ ++pass_4; return true;});

		auto o = s.advance();

		CHECK(1 == pass_1);
		CHECK(1 == pass_2);
		CHECK(1 == pass_3);
		CHECK(1 == pass_4);
		CHECK(STATE_5 == o);
	} // End section : Pass only once




} // End TestCase : state_machine_test_case
