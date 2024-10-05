// MIT License
//
// Copyright (c) 2021 Quentin Balland
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
#include <fil/datastructure/rng.hh>

TEST_CASE("rng_test_case", "[rng]") {
   const std::uint32_t seed = 42;

   SECTION("rng") {
	  fil::rng rnf(seed);

	  CHECK(42 == rnf.get_seed());

	  CHECK(380795 == rnf.generate_in_range(10000, 1000000));
	  CHECK(798578 == rnf.generate_in_range(10000, 1000000));
	  CHECK(951208 == rnf.generate_in_range(10000, 1000000));
	  CHECK(191600 == rnf.generate_in_range(10000, 1000000));
	  CHECK(734674 == rnf.generate_in_range(10000, 1000000));
	  CHECK(781894 == rnf.generate_in_range(10000, 1000000));

   }// End section : rng

   SECTION("uuid generator") {
	  fil::uuid_generator uuid_gen("FyS", seed);

	  CHECK("11205cf7b" == uuid_gen.generate_uuid());
	  CHECK("1120c2f72" == uuid_gen.generate_uuid());
	  CHECK("1120e83a8" == uuid_gen.generate_uuid());
	  CHECK("11202ec70" == uuid_gen.generate_uuid());
	  CHECK("1120b35d2" == uuid_gen.generate_uuid());
	  CHECK("1120bee46" == uuid_gen.generate_uuid());

   }// End section :

}// End TestCase : rng_test_case
