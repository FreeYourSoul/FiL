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

#include <ranges>

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

        CHECK(line.get() == "This is a test file.");
        CHECK(file_reader.load_counter() == 1);

        const auto line2 = file_reader.next_line();
        CHECK(line2.get() == "It has multiple lines.");
        CHECK(file_reader.load_counter() == 1);

        const auto line3 = file_reader.next_line();
        CHECK(line3.get() == "And some more text.");
        CHECK(file_reader.load_counter() == 1);

        SECTION("get_line : read finished : return empty") {
            const auto line4 = file_reader.next_line();
            CHECK(line4.get().empty());
            CHECK(file_reader.load_counter() == 1);
            const auto line5 = file_reader.next_line();
            CHECK(line5.get().empty());
            CHECK(file_reader.load_counter() == 1);
            const auto line6 = file_reader.next_line();
            CHECK(line6.get().empty());
            CHECK(file_reader.load_counter() == 1);
        }
    }

    SECTION("read_until : specific text") {
        const auto found = file_reader.read_until([](std::string_view sv) { return sv.ends_with("It has multiple lines."); });
        CHECK(found.get() == "This is a test file.\nIt has multiple lines.");
        CHECK(file_reader.load_counter() == 1);
    }

    SECTION("read_until : token reader") {
        const auto found_token_1 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_1.get() == "This ");
        CHECK(file_reader.load_counter() == 1);

        const auto found_token_2 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_2.get() == "is ");
        CHECK(file_reader.load_counter() == 1);

        const auto found_token_3 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_3.get() == "a ");
        CHECK(file_reader.load_counter() == 1);

        const auto found_token_4 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_4.get() == "test ");
        CHECK(file_reader.load_counter() == 1);

        const auto found_token_6 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_6.get() == "file.\n");
        CHECK(file_reader.load_counter() == 1);

        const auto found_token_7 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_7.get() == "It ");
        CHECK(file_reader.load_counter() == 1);

        const auto found_token_8 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_8.get() == "has ");
        CHECK(file_reader.load_counter() == 1);

        const auto found_token_9 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_9.get() == "multiple ");
        CHECK(file_reader.load_counter() == 1);

        const auto found_token_10 = file_reader.read_until([](char c) { return c == ' ' || c == '\n'; });
        CHECK(found_token_10.get() == "lines.\n");
        CHECK(file_reader.load_counter() == 1);

        SECTION("read_until : minimum changed, buffer already loaded") {
            const auto token_found_mini = file_reader.read_until([](char c) { return c == '.'; }, 3);
            CHECK(token_found_mini.get() == "And some more text."); // no reload occurred
            CHECK(file_reader.load_counter() == 1);
        }
    }

    SECTION("read_unit : big file") {

        const std::string contentbig = std::ranges::views::iota(0u, fil::READER_BUFFER_SIZE + 3000)    //
                                     | std::ranges::views::transform([](auto) -> char { return 'x'; }) //
                                     | std::ranges::to<std::string>();
        const auto tmp_file_big = std::filesystem::temp_directory_path() / "test_file_2.txt";
        write_file(tmp_file_big, contentbig);

        fil::file_reader file_reader_big(tmp_file_big);

        SECTION("check minimum modified") {
            const auto found_1 = file_reader_big.read_until([i = 1](char) mutable { return ++i == 1000; });
            CHECK(found_1.get().size() == 1000);
            CHECK(file_reader_big.get_file_cursor() == fil::READER_BUFFER_SIZE);
            CHECK(file_reader_big.get_buffer_cursor() == 1000);
            CHECK(file_reader_big.load_counter() == 1);
            CHECK(found_1.is_valid());

            const auto found_changed_mini =
                file_reader_big.read_until([i = 1](char) mutable { return ++i == 1000; }, fil::READER_BUFFER_SIZE + 1001);
            CHECK(found_changed_mini.get().size() == 1000);
            CHECK(file_reader_big.get_buffer_cursor() == 1000); // reload happened - and thus the cursor in the buffer is the same
            CHECK(file_reader_big.get_file_cursor() == fil::READER_BUFFER_SIZE + 1000 /*read 1000 after the cursor*/);
            CHECK(file_reader_big.load_counter() == 2);
            CHECK(!found_1.is_valid());
            CHECK(found_changed_mini.is_valid());
        }

        SECTION("check loading (with too much reading)") {
            const auto found_1 = file_reader_big.read_until([i = 1](char) mutable { return ++i == 1000; });
            CHECK(found_1.get().size() == 1000);
            CHECK(file_reader_big.get_file_cursor() == fil::READER_BUFFER_SIZE);
            CHECK(file_reader_big.get_buffer_cursor() == 1000);
            CHECK(file_reader_big.load_counter() == 1);

            const auto found_2 = file_reader_big.read_until([i = 1](char) mutable { return ++i == 1000; });
            CHECK(found_2.get().size() == 1000);
            CHECK(file_reader_big.get_file_cursor() == fil::READER_BUFFER_SIZE);
            CHECK(file_reader_big.get_buffer_cursor() == 2000);
            CHECK(file_reader_big.load_counter() == 1);

            const auto found_3 = file_reader_big.read_until([i = 1](char) mutable { return ++i == fil::READER_BUFFER_SIZE + 1; });
            CHECK(found_3.get().empty());                       // read failed to find the read
            CHECK(file_reader_big.get_buffer_cursor() == 2000); // didn't move as the read failed
            CHECK(file_reader_big.load_counter() == 1);         // no additional load
        }
    }

    SECTION("read_line : specific line") {

        SECTION("line 0 is invalid") {
            const auto line0 = file_reader.read_line(0);
            CHECK(line0.get().empty());
        }

        const auto last_line = file_reader.read_line(3);
        CHECK(last_line.get() == "And some more text.");
        CHECK(file_reader.load_counter() == 1);
        CHECK(last_line.is_valid()); // line is valid

        const auto middle_line = file_reader.read_line(2);
        CHECK(middle_line.get() == "It has multiple lines.");
        CHECK(file_reader.load_counter() == 2);
        CHECK(!last_line.is_valid());   // the last line becomes invalid
        CHECK(last_line.get().empty()); // the last line becomes empty on get as it is invalid
        CHECK(middle_line.is_valid());

        const auto first_line = file_reader.read_line(1);
        CHECK(first_line.get() == "This is a test file.");
        CHECK(file_reader.load_counter() == 3);
        CHECK(!last_line.is_valid());
        CHECK(!middle_line.is_valid());   // the middle line becomes invalid
        CHECK(middle_line.get().empty()); // the middle line becomes empty on get as it is invalid
        CHECK(first_line.is_valid());

        SECTION("non-existing line") {
            const auto line = file_reader.read_line(4);
            CHECK(line.get().empty());
            CHECK(file_reader.load_counter() == 3);
            CHECK(!last_line.is_valid());
            CHECK(!middle_line.is_valid());
            CHECK(!line.is_valid());
            CHECK(first_line.is_valid());
        }
    }

    SECTION("line_iterator") {
        std::vector<std::string> lines;
        auto it3 = file_reader.make_line_iterator();

        for (auto it = file_reader.make_line_iterator(); it != file_reader.end(); ++it) {
            lines.emplace_back(it->get());
        }
        CHECK(lines.size() == 3);
    }
}