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

    CHECK(file_reader.load_counter() == 0);

    CHECK(file_reader.exists());
    CHECK(file_reader.size() == content.size());

    SECTION("get_line : success") {
        const auto line = file_reader.next_line();

        CHECK(line == "This is a test file.");
        CHECK(file_reader.load_counter() == 1);

        const auto line2 = file_reader.next_line();
        CHECK(line2 == "It has multiple lines.");
        CHECK(file_reader.load_counter() == 1);

        const auto line3 = file_reader.next_line();
        CHECK(line3 == "And some more text.");
        CHECK(file_reader.load_counter() == 1);

        SECTION("get_line : read finished : return empty") {
            const auto line4 = file_reader.next_line();
            CHECK(line4.empty());
            CHECK(file_reader.load_counter() == 1);
            const auto line5 = file_reader.next_line();
            CHECK(line5.empty());
            CHECK(file_reader.load_counter() == 1);
            const auto line6 = file_reader.next_line();
            CHECK(line6.empty());
            CHECK(file_reader.load_counter() == 1);
        }
    }

    SECTION("read_until : specific text") {
        const auto found = file_reader.read_until([](std::string_view sv) { return sv.ends_with("It has multiple lines."); });
        CHECK(found == "This is a test file.\nIt has multiple lines.");
        CHECK(file_reader.load_counter() == 1);
    }

    SECTION("read_until : token reader") {
        const auto found_token_1 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_1 == "This ");
        CHECK(file_reader.load_counter() == 1);

        const auto found_token_2 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_2 == "is ");
        CHECK(file_reader.load_counter() == 1);

        const auto found_token_3 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_3 == "a ");
        CHECK(file_reader.load_counter() == 1);

        const auto found_token_4 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_4 == "test ");
        CHECK(file_reader.load_counter() == 1);

        const auto found_token_6 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_6 == "file.\n");
        CHECK(file_reader.load_counter() == 1);

        const auto found_token_7 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_7 == "It ");
        CHECK(file_reader.load_counter() == 1);

        const auto found_token_8 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_8 == "has ");
        CHECK(file_reader.load_counter() == 1);

        const auto found_token_9 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_9 == "multiple ");
        CHECK(file_reader.load_counter() == 1);

        const auto found_token_10 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_10 == "lines.\n");
        CHECK(file_reader.load_counter() == 1);

        SECTION("read_until : minimum changed, buffer already loaded") {
            const auto token_found_mini = file_reader.read_until([](char c) { return c == '\n'; }, 3);
            CHECK(token_found_mini == "And some more text.");
            CHECK(file_reader.load_counter() == 1);
        }
    }

    SECTION("read_unit : change minimum") {
        const auto found_line1 = file_reader.read_until([](char c) { return c == '\n'; }, 25);
        CHECK(found_line1 == "This is a test file.\n");
    }

    SECTION("read_line : specific line") {

        SECTION("line 0 is invalid") {
            const auto line0 = file_reader.read_line(0);
            CHECK(line0.empty());
        }

        const auto last_line = file_reader.read_line(3);
        CHECK(last_line == "And some more text.");
        CHECK(file_reader.load_counter() == 1);

        const auto middle_line = file_reader.read_line(2);
        CHECK(middle_line == "It has multiple lines.");
        CHECK(file_reader.load_counter() == 2);

        const auto first_line = file_reader.read_line(1);
        CHECK(first_line == "This is a test file.");
        CHECK(file_reader.load_counter() == 3);

        SECTION("non-existing line") {
            const auto line = file_reader.read_line(4);
            CHECK(line.empty());
            CHECK(file_reader.load_counter() == 3);
        }
    }
}