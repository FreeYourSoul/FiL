// Dual Licensing Either:
// - AGPL
// or
// - Subscription license for commercial usage (without requirement of licensing propagation).
//   please contact ballandfys@protonmail.com for additional information about this subscription commercial licensing.
//
// Created by fys on 13.04.26. @Copyright Licensing 2022-2026
//
// In the case no license has been purchased for the use (modification or distribution in any way) of the software stack
// the APGL license is applying.
//

#ifndef FIL_PRINT_ERROR_HH
#define FIL_PRINT_ERROR_HH

#include <fmt/color.h>
#include <fmt/format.h>

#include "rule.hh"

namespace fil::copa {

inline std::string make_string(const file_reader::block_view& block_view) {
    const auto view  = block_view.get();
    const auto itEnd = std::ranges::find(view, '\n');

    if (itEnd == view.end()) {
        return std::string {view};
    }
    return {view.begin(), itEnd};
}

inline std::string make_string(const std::string& line) { return line; }

template<reader Reader>
void print_error(Reader& reader, const error_info& error) {
    auto reader_print = shallow_copy<Reader>::copy(reader);

    const std::size_t read_start = (error.line < 3uz) ? 1uz : error.line - 3uz;

    std::string error_line;
    std::size_t cursor_end_error_line = 0;
    std::string line_str {reader.read_line(read_start).get()};
    for (auto line_number = read_start; line_number < read_start + 6; ++line_number) {

        if (line_number == error.line) {
            error_line            = line_str;
            cursor_end_error_line = reader.reader_cursor();
        }

        const std::string prefix = (line_number == error.line) ? " --> " : "     ";
        fmt::print(stderr, "{}{}{}|{} {}\n", fmt::styled(prefix, fmt::fg(fmt::color::red)),
                   fmt::styled(fmt::format("{:>4}", line_number), fmt::fg(fmt::color::magenta)),
                   fmt::styled("", fmt::fg(fmt::color::dark_gray)), fmt::styled("", fmt::fg(fmt::color::white)),
                   fmt::styled(line_str, fmt::fg(fmt::color::white)));
        line_str = std::string {reader.next_line().get()};
    }

    const auto cursor_begin_line = cursor_end_error_line - error_line.size();
    const auto start_column      = error.cursor - cursor_begin_line;
    const auto end_column        = error.cursor - cursor_begin_line + error.token_failure.size();
    const auto error_length      = end_column - start_column + 1;

    // Print the error line with highlighting
    fmt::print(stderr, " {:>4}\n",
               fmt::styled("----------------------------------------------------------------", fmt::fg(fmt::color::aquamarine)));
    fmt::print(stderr, "{} --> ", fmt::styled("", fmt::fg(fmt::color::red)));
    fmt::print(stderr, "{:>4}{}|{} ", fmt::styled(error.line, fmt::fg(fmt::color::magenta)),
               fmt::styled("", fmt::fg(fmt::color::dark_gray)), fmt::styled("", fmt::fg(fmt::color::white)));

    for (int i = 0; i < static_cast<int>(error_line.length()); ++i) {
        if (i >= start_column - 1 && i < start_column - 1 + error_length) {
            fmt::print(stderr, "{}", fmt::styled(error_line[i], fmt::fg(fmt::color::red) | fmt::emphasis::bold));
        } else {
            fmt::print(stderr, "{}", fmt::styled(error_line[i], fmt::fg(fmt::color::white)));
        }
    }
    fmt::print(stderr, "\n");

    // Print underline pointer
    fmt::print(stderr, " {:>4} {} ", fmt::styled("", fmt::fg(fmt::color::dark_gray)), fmt::styled("", fmt::fg(fmt::color::white)));
    for (int i = 0; i < start_column; ++i) {
        fmt::print(stderr, " ");
    }
    fmt::print(stderr, "   {}\n", fmt::styled(std::string(error_length, '^'), fmt::fg(fmt::color::red) | fmt::emphasis::bold));

    // Print error message and detailed explanation
    fmt::print(stderr, "\n{}: {}\n", fmt::styled("error during step", fmt::fg(fmt::color::red) | fmt::emphasis::bold), error.parsing_step);
    fmt::print(stderr, "{}\n", fmt::styled(fmt::format("→ {}", error.error_msg), fmt::fg(fmt::color::yellow)));
}
} // namespace fil::copa
#endif // FIL_PRINT_ERROR_HH
