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
#include <fil/algorithm/contains.hh>
#include <fil/algorithm/string.hh>
#include <fil/algorithm/suitable.hh>
#include <tuple>

TEST_CASE("algorithm_testcase find_more_suitable", "[algorithm]") {

   SECTION("Empty") {
	  std::vector<int> basic = {};
	  auto min = [](int curr, int next) { return curr > next; };

	  REQUIRE(basic.end() == fil::find_most_suitable(basic.begin(), basic.end(), min));
   }

   SECTION("Basic test") {
	  std::vector<int> basic = {1, 3, 4, 5, 2, 1, 5, 6, 1, 33, 22, 1111, 34, 5};

	  auto min = [](int curr, int next) { return curr > next; };
	  auto max = [](int curr, int next) { return curr < next; };
	  REQUIRE(basic.end() != fil::find_most_suitable(basic.begin(), basic.end(), max));
	  REQUIRE(11 == std::distance(basic.begin(), fil::find_most_suitable(basic.begin(), basic.end(), max)));
	  REQUIRE(1111 == *fil::find_most_suitable(basic.begin(), basic.end(), max));
	  REQUIRE(basic.end() != fil::find_most_suitable(basic.begin(), basic.end(), min));
	  REQUIRE(0 == std::distance(basic.begin(), fil::find_most_suitable(basic.begin(), basic.end(), min)));
	  REQUIRE(1 == *fil::find_most_suitable(basic.begin(), basic.end(), min));
   }

   SECTION("Structure") {

	  struct TestingStruct {
		 int a;
		 std::string b;
		 double c;

		 bool
		 operator==(const TestingStruct& other) const {
			return std::make_tuple(a, b, c)
				== std::make_tuple(other.a, other.b, other.c);
		 }
	  };

	  std::vector<TestingStruct> structVec =
		  {
			  {1, "salutzkk", 1.1},
			  {1, "salutf3f3f3f3f", 1.1},
			  {1, "salutf32", 1.1},
			  {1, "salut3", 1.1}};

	  TestingStruct longest {1, "salutf3f3f3f3f", 1.1};
	  TestingStruct smallest {1, "salut3", 1.1};

	  auto findLong = [](auto& curr, auto& next) { return curr.b.size() < next.b.size(); };
	  auto findShort = [](auto& curr, auto& next) { return curr.b.size() > next.b.size(); };

	  REQUIRE(structVec.end() != fil::find_most_suitable(structVec.begin(), structVec.end(), findLong));
	  REQUIRE(1 == std::distance(structVec.begin(), fil::find_most_suitable(structVec.begin(), structVec.end(), findLong)));
	  REQUIRE(longest == *fil::find_most_suitable(structVec.begin(), structVec.end(), findLong));
	  REQUIRE(structVec.end() != fil::find_most_suitable(structVec.begin(), structVec.end(), findShort));
	  REQUIRE(3 == std::distance(structVec.begin(), fil::find_most_suitable(structVec.begin(), structVec.end(), findShort)));
	  REQUIRE(smallest == *fil::find_most_suitable(structVec.begin(), structVec.end(), findShort));
   }

}// end testcase : algorithm_testcase find_more_suitable

TEST_CASE("algorithm_testcase all_contains", "[algorithm]") {

   struct TestingType {

	  bool operator==(const TestingType& other) const { return name == other.name && id == other.id; }

	  std::string name;
	  std::uint32_t id;
   };

   std::vector<TestingType> initial_vector =
	   {{"name1", 1},
		{"name2", 2},
		{"name3", 3},
		{"name4", 4},
		{"name5", 6},
		{"name7", 7},
		{"name8", 8}};

   SECTION("All in vector : Simple ") {

	  std::vector<TestingType> success =
		  {{"name8", 8},
		   {"name3", 3},
		   {"name1", 1}};

	  // All element in success are in initial_vector
	  CHECK(fil::all_contains(success, initial_vector));

	  // opposite is not true
	  CHECK_FALSE(fil::all_contains(initial_vector, success));

	  std::vector<TestingType> failure =
		  {{"name8", 8},
		   {"name3", 3},
		   {"name42", 42}};

	  // element of id 42 isn't present in initial_vector
	  CHECK_FALSE(fil::all_contains(failure, initial_vector));

   }// End section : All in vector

   SECTION("All in vector : custom Accessor") {
	  std::vector<TestingType> success =
		  {{"name8", 3423423},
		   {"name3", 111212},
		   {"name1", 23244}};

	  // All element NAME in success are in initial_vector (id mismatch but are not checked)
	  CHECK(fil::all_contains(success, initial_vector,
							  [](const auto& elem) -> const std::string& { return elem.name; }));

	  success.emplace_back(TestingType {"nannnan", 22342});

	  // nannnan doesn't exist in initial_vector
	  CHECK_FALSE(fil::all_contains(success, initial_vector,
									[](const auto& elem) -> const std::string& { return elem.name; }));

   }// End section : All in vector : custom Accessor

   SECTION("All in vector : custom Accessor And different vec type") {

	  struct AnotherType {
		 std::string chocobo {};
		 std::uint32_t pepe {0};
	  };

	  std::vector<AnotherType> success =
		  {{"name1", 1}, {"name2", 2}, {"name3", 3}, {"name4", 4}, {"name5", 6}, {"name6", 6}, {"name7", 7}, {"name8", 8}, {"name42", 42}};

	  CHECK(fil::all_contains(initial_vector, success,
							  [](const AnotherType& elem) { return TestingType {elem.chocobo, elem.pepe}; }));

	  initial_vector.emplace_back(TestingType {"nannnan", 22342});

	  CHECK_FALSE(fil::all_contains(initial_vector, success,
									[](const auto& elem) { return TestingType {elem.chocobo, elem.pepe}; }));

   }// End section : All in vector : custom Accessor  And different vec type

}// end testcase : algorithm_testcase all_contains

TEST_CASE("algorithm_testcase string", "[algorithm]") {

   SECTION("basic split_string test") {

	  std::uint32_t counter = 0;
	  fil::split_string("this;contains;three;semi-column", ";", [&counter](const std::string& fragment) { ++counter; });
	  REQUIRE(4 == counter);

   }// End section : basic split_string test

   SECTION("split_string no occurrence") {

	  std::uint32_t counter = 0;
	  std::string value;
	  fil::split_string("this", ";", [&counter, &value](const std::string& fragment) { ++counter; value = fragment; });
	  REQUIRE(1 == counter);
	  REQUIRE("this" == value);

   }// End section : basic split_string test

   SECTION("split_string no occurrence") {

	  std::uint32_t counter = 0;
	  std::vector<std::string> values;
	  fil::split_string(";", ";", [&counter, &values](const std::string& fragment) { ++counter; values.emplace_back(fragment); });
	  REQUIRE(2 == counter);
	  REQUIRE(values.at(0).empty());
	  REQUIRE(values.at(1).empty());

   }// End section : basic split_string test

   SECTION("basic split_string check fragment") {

	  std::vector<std::string> fragments;
	  fil::split_string("this;contains;three;semi-column", ";", [&fragments](std::string fragment) { fragments.emplace_back(std::move(fragment)); });
	  REQUIRE(4 == fragments.size());
	  REQUIRE("this" == fragments.at(0));
	  REQUIRE("contains" == fragments.at(1));
	  REQUIRE("three" == fragments.at(2));
	  REQUIRE("semi-column" == fragments.at(3));

   }// End section : basic split_string test

   SECTION("basic split_string check fragment with limitation") {

	  std::vector<std::string> fragments;
	  fil::split_string(
		  "this;contains;three;semi-column", ";", [&fragments](std::string fragment) { fragments.emplace_back(std::move(fragment)); }, 2);
	  REQUIRE(2 == fragments.size());
	  REQUIRE("this" == fragments.at(0));
	  REQUIRE("contains;three;semi-column" == fragments.at(1));

   }// End section : basic split_string test

   SECTION("basic split_string long separator") {

	  std::vector<std::string> fragments;
	  fil::split_string("this[-----]contains[-----]three[-----]semi-column", "[-----]",
						[&fragments](std::string fragment) { fragments.emplace_back(std::move(fragment)); });
	  REQUIRE(4 == fragments.size());
	  REQUIRE("this" == fragments.at(0));
	  REQUIRE("contains" == fragments.at(1));
	  REQUIRE("three" == fragments.at(2));
	  REQUIRE("semi-column" == fragments.at(3));

   }// End section : basic split_string test

   SECTION("basic split_string multi separator check fragment") {

	  std::vector<std::string> fragments;
	  fil::split_string("this;contains:three?semi-column", {"?", ":", ";"}, [&fragments](std::string fragment) { fragments.emplace_back(std::move(fragment)); });
	  REQUIRE(4 == fragments.size());
	  REQUIRE("this" == fragments.at(0));
	  REQUIRE("contains" == fragments.at(1));
	  REQUIRE("three" == fragments.at(2));
	  REQUIRE("semi-column" == fragments.at(3));

   }// End section : basic split_string test

   SECTION("basic split_string multi separator long separator") {

	  std::vector<std::string> fragments;
	  fil::split_string("this//////contains:three[-----]semi-column", {"[-----]", ":", "//////"},
						[&fragments](std::string fragment) { fragments.emplace_back(std::move(fragment)); });
	  REQUIRE(4 == fragments.size());
	  REQUIRE("this" == fragments.at(0));
	  REQUIRE("contains" == fragments.at(1));
	  REQUIRE("three" == fragments.at(2));
	  REQUIRE("semi-column" == fragments.at(3));

   }// End section : basic split_string test

   SECTION("basic split_string multi separator long separator with limitation") {

	  std::vector<std::string> fragments;
	  fil::split_string(
		  "this//////contains:three[-----]semi-column", {"[-----]", ":", "//////"},
		  [&fragments](std::string fragment) { fragments.emplace_back(std::move(fragment)); }, 3);
	  REQUIRE(3 == fragments.size());
	  REQUIRE("this" == fragments.at(0));
	  REQUIRE("contains" == fragments.at(1));
	  REQUIRE("three[-----]semi-column" == fragments.at(2));

   }// End section : basic split_string test

   SECTION("string join") {

	  std::vector<std::string> strings {"this", "is", "a", "string", "split"};

	  CHECK("thisisastringsplit" == fil::join(strings));

	  CHECK("this is a string split" == fil::join(strings, " "));

	  CHECK("this | | is | | a | | string | | split" == fil::join(strings, " | | "));

   }// End section : string join

}// end testcase : algorithm_testcase string