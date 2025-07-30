// Dual Licensing Either :
// - AGPL
// or
// - Subscription license for commercial usage (without requirement of licensing propagation).
//   please contact ballandfys@protonmail.com for additional information about this subscription commercial licensing.
//
// Created by FyS on 29.07.25. License 2022-2025
//
// In the case no license has been purchased for the use (modification or distribution in any way) of the software stack
// the APGL license is applying.
//

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fmt/format.h>

#include "fil/file/file_reader.hh"

namespace {

void write_file(const std::filesystem::path& file_path, const std::string& content) {
    std::ofstream file(file_path);
    if (!file) {
        throw std::runtime_error(fmt::format("Could not open file for writing: {}", file_path.c_str()));
    }
    file << content;
    if (!file) {
        throw std::runtime_error(fmt::format("Error writing to file: ", file_path.c_str()));
    }
}

} // namespace

TEST_CASE("read_file_testcase", "[file]") {

    const auto tmp_file       = std::filesystem::temp_directory_path() / "test_file.txt";
    const std::string content = "This is a test file.\nIt has multiple lines.\nAnd some more text.";
    write_file(tmp_file, content);

    fil::file_reader file_reader(tmp_file);

    CHECK(file_reader.exists());
    CHECK(file_reader.size() == content.size());

    SECTION("get_line : success") {
        const auto line = file_reader.next_line();
        CHECK(line == "This is a test file.");

        const auto line2 = file_reader.next_line();
        CHECK(line2 == "It has multiple lines.");

        const auto line3 = file_reader.next_line();
        CHECK(line3 == "And some more text.");

        SECTION("get_line : read finished : return empty") {
            const auto line4 = file_reader.next_line();
            CHECK(line4.empty());
            const auto line5 = file_reader.next_line();
            CHECK(line5.empty());
            const auto line6 = file_reader.next_line();
            CHECK(line6.empty());
        }
    }

    SECTION("read_until : specific text") {
        const auto found = file_reader.read_until([](std::string_view sv) { return sv.ends_with("It has multiple lines."); });
        CHECK(found == "This is a test file.\nIt has multiple lines.");
    }

    SECTION("read_until : token reader") {
        const auto found_token_1 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_1 == "This ");

        const auto found_token_2 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_2 == "is ");

        const auto found_token_3 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_3 == "a ");

        const auto found_token_4 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_4 == "test ");

        const auto found_token_6 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_6 == "file.\n");

        const auto found_token_7 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_7 == "It ");

        const auto found_token_8 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_8 == "has ");

        const auto found_token_9 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_9 == "multiple ");

        const auto found_token_10 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_10 == "lines.\n");
    }

    SECTION("go_to : specific line") {

        SECTION("line 0 is invalid") {
            const auto line0 = file_reader.read_line(0);
            CHECK(line0.empty());
        }

        const auto first_line = file_reader.read_line(1);
        CHECK(first_line == "This is a test file.");

        const auto last_line = file_reader.read_line(3);
        CHECK(last_line == "And some more text.");

        const auto middle_line = file_reader.read_line(2);
        CHECK(middle_line == "It has multiple lines.");

        SECTION("non-existing line") {
            const auto line = file_reader.read_line(4);
            CHECK(line.empty());
        }
    }
}