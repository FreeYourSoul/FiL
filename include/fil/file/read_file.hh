// Dual Licensing Either :
// - AGPL
// or
// - Subscription license for commercial usage (without requirement of licensing propagation).
//   please contact ballandfys@protonmail.com for additional information about this subscription commercial licensing.
//
// Created by FyS on 23.07.25. License 2022-2025
//
// In the case no license has been purchased for the use (modification or distribution in any way) of the software stack
// the APGL license is applying.
//

#ifndef FILE_H
#define FILE_H

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fil {

static constexpr std::uint32_t MAX_SIZE_BUFFER = 1024 * 1024; //!< max size of the buffer used to read the file (1Mb)
//! end position in the buffer to read from (-1 to have an extra byte for null-termination)
static constexpr std::uint32_t BUFFER_SIZE = MAX_SIZE_BUFFER - 1;

class file_reader {
  public:
    class sentinel {};
    class line_iterator;

    explicit file_reader(const std::filesystem::path& file_path)
        : file_path_(file_path)
        , file_stream_(file_path_, std::ios::ate | std::ios::binary)
        , size_(!file_stream_ ? 0 : file_stream_.tellg()) {

        if (size_ == 0) {
            return; // stream could not be opened, possibly file does not exist
        }
        current_buffer_.reserve(size_ % MAX_SIZE_BUFFER);

        go_to(0);
    }

    std::string_view go_to(std::uint32_t line) {
        file_stream_.seekg(std::ios::beg);

        for (std::uint32_t i = 0; i < line - 1; ++i) {
            file_stream_.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        load_();

        return line();
    }

    [[nodiscard]] char peek() const {
        if (cursor_ >= current_buffer_.size()) {
            return '\0'; // No more data to read
        }
        return current_buffer_[cursor_];
    }

    [[nodiscard]] std::string_view read(std::uint32_t size, std::uint32_t from = 0) {
        if (current_buffer_.empty()) {
            load_();
        }
        // const auto pos_to = [start = from + size, max_pos = static_cast<std::uint32_t>(current_buffer_.size() - 1)] ->
        // std::optional<std::uint32_t> { //
        //     if ()
        //     return std::min(start, max_pos);
        // }();

        cursor_ = pos_to;
    }

    [[nodiscard]] std::string_view next_line() {
        if (current_buffer_.empty()) {
            load_();
        }

        if (current_buffer_.empty()) {
            return std::string_view {};
        }

        const auto pos       = std::find(current_buffer_.begin() + cursor_, current_buffer_.end(), [](char c) { return c == '\n'; });
        const auto size_line = std::distance(current_buffer_.begin() + cursor_, pos);

        std::string_view line {current_buffer_.data() + cursor_, size_line};

        cursor_ = std::distance(current_buffer_.begin(), pos) + 1; // move past the newline character

        return line;
    }
    [[nodiscard]] const std::filesystem::path& get_path() const { return file_path_; }
    [[nodiscard]] bool exists() const { return std::filesystem::exists(file_path_); }
    [[nodiscard]] auto get_file_cursor() { return file_stream_.tellg(); }
    [[nodiscard]] bool size() const { return size_; }

  private:
    void load_() {
        current_buffer_.clear();
        cursor_ = 0;

        if (!file_stream_.good() || file_stream_.eof()) {
            return; // No more data to read
        }

        file_stream_.read(current_buffer_.data(), BUFFER_SIZE);
        current_buffer_[file_stream_.gcount()] = '\0'; // Null-terminate the string
    }

  private:
    std::filesystem::path file_path_; //!< path to the file to read

    std::ifstream file_stream_;       //!< file stream to read from
    std::string current_buffer_ {};   //!< buffer of the current read
    std::uint32_t cursor_ {0};        //!< cursor in the buffer of the current line

    std::uint32_t size_ {0};          //!< file size in bytes
};

/**
 * @brief iterator through line of a read_file
 */
class file_reader::line_iterator {
  public:
    line_iterator(file_reader* file_handler, std::uint32_t offset)
        : file_handler_(file_handler) {}

    line_iterator& operator++() {
        if (!file_handler_ || offset_ >= file_handler_->size()) {
            return *this;
        }

        ++offset_;
        return *this;
    }

    auto operator!=(const line_iterator& other) const { return offset_ != other.offset_; }
    bool operator!=(const file_reader::sentinel& other) const { return offset_ >= file_handler_->size(); }
    std::string_view operator*() const { return line_; }

  private:
    file_reader* file_handler_;
    std::string_view line_ {0};
};

[[nodiscard]] inline file_reader::line_iterator begin(file_reader& file) { return {&file, 0}; }
[[nodiscard]] inline file_reader::sentinel end(file_reader& file) { return {}; }

} // namespace fil

#endif // FILE_H
