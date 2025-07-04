
// - AGPL
// or
// - Subscription license for commercial usage (without requirement of licensing propagation).
//   please contact ballandfys@protonmail.com for additional information about this subscription commercial licensing.
//
// Copyright (c) 2022-2024.
// Created by fys on 05.10.24.
//
// In the case no license has been purchased for the use (modification or distribution in any way) of the software stack
// the APGL license is applying.

//
// Created by fys on 05.10.24.
//

#include <catch2/catch_test_macros.hpp>
#include <fil/datastructure/soa.hh>

TEST_CASE("soa_vector", "[datastructure]") {

    fil::soa<int, double, std::string> s {};

    CHECK(s.is_empty());

    SECTION("test_reserve") {
        s.reserve(10z);
        CHECK(s.size() == 0);  // size should be 0 after reserve

        const auto id = s.insert(1, 1.1, "one");
        CHECK(s.size() == 1);  // size should be 1 after insert
        CHECK(id.offset == 0); // offset should be 0 for the first element even after a reserve
        CHECK(s.has_id(id));   // should have the id we just inserted
    }

    SECTION("test_insert") {

        const auto id1 = s.insert(1, 1.1, "one");
        const auto id2 = s.insert(2, 2.2, "two");
        const auto id3 = s.insert(3, 3.3, "three");

        CHECK(id1.offset == 0);
        CHECK(id1.generation == 0);

        CHECK(id2.offset == 1);
        CHECK(id2.generation == 0);

        CHECK(id3.offset == 2);
        CHECK(id3.generation == 0);

        CHECK(s.size() == 3);
    }

    const std::array int_values {1, 2, 3, 4};
    const std::array double_values {1.1, 2.2, 3.3, 4.4};
    const std::array string_values {std::string {"one"}, std::string {"two"}, std::string {"three"}, std::string {"four"}};

    const auto id1 = s.insert(int_values[0], double_values[0], string_values[0]);
    const auto id2 = s.insert(int_values[1], double_values[1], string_values[1]);
    const auto id3 = s.insert(int_values[2], double_values[2], string_values[2]);
    const auto id4 = s.insert(int_values[3], double_values[3], string_values[3]);

    SECTION("direct_access_structure_binding") {
        const auto& [integer0, double_value0, string_value0] = s[id1];

        CHECK(int_values[0] == integer0);
        CHECK(double_values[0] == double_value0);
        CHECK(string_values[0] == string_value0);

        const auto& [integer1, double_value1, string_value1] = s[id2];

        CHECK(int_values[1] == integer1);
        CHECK(double_values[1] == double_value1);
        CHECK(string_values[1] == string_value1);

        const auto& [integer2, double_value2, string_value2] = s[id3];

        CHECK(int_values[2] == integer2);
        CHECK(double_values[2] == double_value2);
        CHECK(string_values[2] == string_value2);

        const auto& [integer3, double_value3, string_value3] = s[id4];

        CHECK(int_values[3] == integer3);
        CHECK(double_values[3] == double_value3);
        CHECK(string_values[3] == string_value3);
    }

    SECTION("test_loop") {
        int i = 0;
        SECTION("iterator") {
            for (auto it = s.begin(); it != s.end(); ++it) {
                using std::get;
                const auto& int_value    = get<0>(*it);
                const auto& double_value = get<1>(*it);
                const auto& string_value = get<2>(*it);

                CHECK(int_values[i] == int_value);
                CHECK(double_values[i] == double_value);
                CHECK(string_values[i] == string_value);
                ++i;
            }
        }
        SECTION("test_for_loop") {
            for (auto value : s) {
                using std::get;
                const auto& int_value    = get<0>(value);
                const auto& double_value = get<1>(value);
                const auto& string_value = get<2>(value);

                CHECK(int_values[i] == int_value);
                CHECK(double_values[i] == double_value);
                CHECK(string_values[i] == string_value);
                ++i;
            }
        }
        SECTION("for_loop_structure_binding") {
            for (const auto& [int_value, double_value, string_value] : s) {
                CHECK(int_values[i] == int_value);
                CHECK(double_values[i] == double_value);
                CHECK(string_values[i] == string_value);
                ++i;
            }
        }
    }

    SECTION("test_algorithm_usage") {
        const auto v = std::ranges::fold_left(s, 0z, [](std::size_t res, const auto& v) {
            const auto& [i, d, str] = v;
            return res + i;
        });
        CHECK(v == 10); // 1 + 2 + 3 + 4 = 10
    }

    SECTION("test_soa_apply") {
        const auto it = std::ranges::find_if( //
            s, fil::soa_apply([](int i, double, const std::string&) { return i == 3; }));

        CHECK(it != s.end());
        auto [i, d, str] = *it;
        CHECK(i == 3);
        CHECK(d == 3.3);
        CHECK(str == "three");
    }

    SECTION("test_soa_select") {
        const auto it = std::ranges::find_if( //
            s, fil::soa_select<2>([](const std::string& str) { return str == "three"; }));

        CHECK(it != s.end());
        auto [i, d, str] = *it;
        CHECK(i == 3);
        CHECK(d == 3.3);
        CHECK(str == "three");
    }

    SECTION("test_erase") {
        CHECK(s.erase(id3));

        CHECK(s.size() == 3);
        CHECK(s.has_id(id1));
        CHECK(s.has_id(id2));
        CHECK_FALSE(s.has_id(id3));
        CHECK(s.has_id(id4));

        SECTION("test_erase_non_existing_id") { // id3 was previously removed and does not exist anymore
            CHECK(s.erase(id3) == false);
        }

        SECTION("insert_after_erase") {
            // an insertion after an erase should use the next free index (which is id3) and increment the generation
            const auto id3_bis = s.insert(50, 50.50, "fifty");
            CHECK(id3.offset == 2);         // reminder that id3 was offset 2
            CHECK(id3_bis.offset == 2);     // id3 was removed, so the next free index is 2 -- which is used by the newly inserted id3_bis
            CHECK(id3.generation == 0);     // id3 was not modified, so the generation is still 0
            CHECK(id3_bis.generation == 1); // id3_bis was modified, so the generation is now 1
        }

        SECTION("using_old_id_after_erase") {
            //@todo : implement a at function
        }
        SECTION("test_erase_all") {
            CHECK(s.erase(id1));
            CHECK(s.erase(id2));
            CHECK(s.erase(id4));
            CHECK(s.is_empty());
        }
    }

    SECTION("modifications") {

        const auto it = std::ranges::find_if( //
            s, fil::soa_select<2>([](const std::string& str) { return str == "two"; }));

        auto struct_soa                = *it;
        auto& [int_v, double_v, str_v] = struct_soa;

        CHECK(int_v == 2);
        CHECK(str_v == "two");

        SECTION("direct retrieve") { get<0>(*it) = 1337; }
        SECTION("structure binding") { int_v = 1337; }

        const auto check = std::ranges::find_if( //
            s, fil::soa_select<2>([](const std::string& str) { return str == "two"; }));

        const auto& [int_v_bis, double_v_bis, str_v_bis] = *check;

        CHECK(int_v_bis == 1337);
        CHECK(str_v_bis == "two");
    }
}