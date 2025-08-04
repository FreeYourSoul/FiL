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
#include <functional>

namespace fil {

static constexpr std::uint32_t BUFFER_SIZE = 1024 * 1024 + 1; //!< max size of the buffer used to read the file (1Mb)
//! end position in the buffer to read from (-1 to have an extra byte for null-termination)

class file_reader {
  public:
    class sentinel {};
    class line_iterator;

    explicit file_reader(const std::filesystem::path& file_path)
        : file_path_(file_path)
        , file_stream_(file_path_, std::ios::in | std::ios::binary)
        , size_([this] -> std::uint32_t {
            if (file_stream_.bad())
                return 0;
            file_stream_.seekg(0, std::ios::end);
            const auto size = static_cast<std::uint32_t>(file_stream_.tellg());
            file_stream_.seekg(0, std::ios::beg);
            return size;
        }()) {
        const auto size_buffer = std::min(size_, BUFFER_SIZE);
        current_buffer_.resize(size_buffer);
    }

    std::string_view read_line(std::uint32_t line) {
        buffer_size_ = 0;
        file_stream_.clear();
        file_stream_.seekg(0, std::ios::beg);

        for (std::uint32_t i = 0; i < line - 1; ++i) {
            file_stream_.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (file_stream_.eof() || file_stream_.fail()) {
                return {};
            }
        }
        load_();

        return next_line();
    }

    template<std::invocable<std::string_view> Predicate>
    [[nodiscard]] std::string_view read_until(Predicate&& predicate, std::size_t minimum_size = 100) {
        if (!current_buffer_.front() != '\0' || ((buffer_size_ - cursor_) <= minimum_size)) {
            load_();
        }
        const auto start_cursor = cursor_;
        while (cursor_ < buffer_size_) {
            std::string_view sv = {current_buffer_.begin() + start_cursor, current_buffer_.begin() + ++cursor_};
            if (std::invoke(predicate, sv)) {
                return sv;
            }
        }
        return {current_buffer_.begin() + start_cursor, current_buffer_.end()};
    }

    template<std::invocable<char> Predicate>
    [[nodiscard]] std::string_view read_until(Predicate&& predicate, std::size_t minimum_size = 100) {
        if (!current_buffer_.front() != '\0' || ((buffer_size_ - cursor_) <= minimum_size)) {
            load_();
        }
        const auto start_cursor = cursor_;
        while (cursor_ < buffer_size_) {
            if (std::invoke(predicate, current_buffer_[++cursor_])) {
                return {current_buffer_.begin() + start_cursor, current_buffer_.begin() + ++cursor_};
            }
        }
        return {current_buffer_.begin() + start_cursor, current_buffer_.end()};
    }

    [[nodiscard]] std::string_view next_line() {
        if (cursor_ >= buffer_size_) {
            load_();
        }

        if (buffer_size_ == 0 || cursor_ >= buffer_size_) {
            return std::string_view {};
        }

        const auto pos =
            std::find_if(current_buffer_.begin() + cursor_, current_buffer_.end(), [](auto c) { return c == '\n' || c == '\0'; });
        const auto size_line = std::distance(current_buffer_.begin() + cursor_, pos);

        std::string_view line {current_buffer_.data() + cursor_, static_cast<std::size_t>(size_line)};

        cursor_ = std::distance(current_buffer_.begin(), pos) + 1; // move past the newline character

        return line;
    }

    [[nodiscard]] const std::filesystem::path& get_path() const { return file_path_; }
    [[nodiscard]] bool exists() const { return std::filesystem::exists(file_path_); }
    [[nodiscard]] auto get_file_cursor() { return file_stream_.tellg(); }
    [[nodiscard]] auto size() const { return size_; }
    [[nodiscard]] std::size_t load_counter() const { return load_counter_; }

  private:
    void load_() {
        if (file_stream_.eof())
            return;

        if (buffer_size_ != 0) {
            // move cursor backward to the last read position to retrieve the same leftover of buffer that was not read
            if (cursor_ < current_buffer_.size())
                file_stream_.seekg(-(current_buffer_.size() - cursor_), std::ios::cur);
        }
        ++load_counter_;

        buffer_size_ = 0;
        cursor_      = 0;

        if (!file_stream_.good() || file_stream_.eof()) {
            return; // No more data to read
        }

        file_stream_.read(current_buffer_.data(), BUFFER_SIZE);
        buffer_size_                  = file_stream_.gcount();
        auto d                        = current_buffer_.size();
        current_buffer_[buffer_size_] = '\0';
    }

  private:
    std::filesystem::path file_path_; //!< path to the file to read

    std::ifstream file_stream_;       //!< file stream to read from
    std::string current_buffer_ {};   //!< buffer of the current read
    std::size_t buffer_size_ {0};     //!< size of the buffer
    std::uint32_t cursor_ {0};        //!< cursor in the buffer of the current line

    std::uint32_t size_ {0};          //!< file size in bytes
    std::size_t load_counter_ {0};    //!< counter to inform on how many load occurred
};

/**
 * @brief iterator through line of a read_file
 */
// class file_reader::line_iterator {
//   public:
//     line_iterator(file_reader* file_handler, std::uint32_t offset)
//         : file_handler_(file_handler) {}
//
//     line_iterator& operator++() {
//         if (!file_handler_ || offset_ >= file_handler_->size()) {
//             return *this;
//         }
//
//         ++offset_;
//         return *this;
//     }
//
//     auto operator!=(const line_iterator& other) const { return offset_ != other.offset_; }
//     bool operator!=(const file_reader::sentinel& other) const { return offset_ >= file_handler_->size(); }
//     std::string_view operator*() const { return line_; }
//
//   private:
//     file_reader* file_handler_;
//     std::string_view line_ {0};
// };

// [[nodiscard]] inline file_reader::line_iterator begin(file_reader& file) { return {&file, 0}; }
// [[nodiscard]] inline file_reader::sentinel end(file_reader& file) { return {}; }

} // namespace fil

#endif // FILE_H
