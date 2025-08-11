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
#include <utility>

namespace fil {

static constexpr std::size_t READER_BUFFER_SIZE = 1024 * 1024 + 1; //!< max size of the buffer used to read the file (1Mb)
//! end position in the buffer to read from (-1 to have an extra byte for null-termination)

class file_reader {
  public:
    /**
     * @brief represent a block of text retrieved from the file_reader.
     * A block of text is a view on the buffer of the file_reader. The main purpose of this class is to ensure that the block is still
     * valid at the moment of usage.
     */
    class block_view {
        friend class file_reader;

      private:
        block_view() = default;
        block_view(const std::string_view& block, std::size_t load_id_block, file_reader* reader)
            : block_(block)
            , load_id_block_(load_id_block)
            , reader_(reader) {}

      public:
        /**
         * @return true if the line is still valid, false otherwise
         */
        [[nodiscard]] bool is_valid() const { return reader_ != nullptr && reader_->load_counter() == load_id_block_; }

        /**
         * @return string view representing the line of the
         */
        [[nodiscard]] std::string_view get() const { return is_valid() ? block_ : std::string_view {}; }

      private:
        std::string_view block_ {};     //!< line retrieved from the buffer
        std::size_t load_id_block_ {0}; //!< load id that contains that block of text
        file_reader* reader_ {};        //!< pointer to the file reader that contains the line
    };

    template<std::invocable<std::string_view>> class iterator_file_ {};

    class sentinel {};   //!< sentinel for end iteration
    class line_iterator; //!< iterator that iterate through lines in the file

    explicit file_reader(std::filesystem::path file_path)
        : file_path_(std::move(file_path))
        , file_stream_(file_path_, std::ios::in | std::ios::binary)
        , size_([this] -> std::size_t {
            if (file_stream_.bad())
                return 0;
            file_stream_.seekg(0, std::ios::end);
            const auto size = static_cast<std::uint32_t>(file_stream_.tellg());
            file_stream_.seekg(0, std::ios::beg);
            return size;
        }()) {
        const auto size_buffer = std::min(size_, READER_BUFFER_SIZE);
        current_buffer_.resize(size_buffer);
    }

    block_view read_line(std::size_t line) {
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
    [[nodiscard]] block_view read_until(Predicate&& predicate, std::size_t minimum_size = 100) {
        if (!current_buffer_.front() != '\0' || ((buffer_size_ - cursor_) <= minimum_size)) {
            load_();
        }
        const auto start_cursor = cursor_;
        while (cursor_ < buffer_size_) {
            std::string_view sv = {current_buffer_.begin() + start_cursor, current_buffer_.begin() + ++cursor_};
            if (std::invoke(predicate, sv)) {
                return {sv, load_counter_, this};
            }
        }
        cursor_ = start_cursor;
        return {};
    }

    template<std::invocable<char> Predicate>
    [[nodiscard]] block_view read_until(Predicate&& predicate, std::size_t minimum_size_reload = 100) {
        if (buffer_size_ == 0 || ((buffer_size_ - cursor_) <= minimum_size_reload)) {
            load_();
        }
        const auto start_cursor = cursor_;
        while (cursor_ < buffer_size_) {
            if (std::invoke(predicate, current_buffer_[++cursor_])) {
                return {
                    {current_buffer_.begin() + start_cursor, current_buffer_.begin() + ++cursor_},
                    load_counter_, this
                };
            }
        }
        cursor_ = start_cursor;
        return {};
    }

    [[nodiscard]] block_view next_line() {
        if (cursor_ >= buffer_size_) {
            load_();
        }

        if (buffer_size_ == 0 || cursor_ >= buffer_size_) {
            cursor_ = std::numeric_limits<decltype(cursor_)>::max(); // force cursor at "end" for iterator
            return {};
        }

        const auto pos =
            std::find_if(std::next(current_buffer_.begin(), cursor_), current_buffer_.end(), [](auto c) { return c == '\n' || c == '\0'; });
        const auto size_line = std::distance(std::next(current_buffer_.begin(), cursor_), pos);

        std::string_view line {current_buffer_.data() + cursor_, static_cast<std::size_t>(size_line)};

        cursor_ = std::distance(current_buffer_.begin(), pos) + 1; // move past the newline character

        return {line, load_counter_, this};
    }

    [[nodiscard]] const std::filesystem::path& get_path() const { return file_path_; }
    [[nodiscard]] bool exists() const { return std::filesystem::exists(file_path_); }
    [[nodiscard]] auto get_file_cursor() { return file_stream_.tellg(); }
    [[nodiscard]] std::size_t get_buffer_cursor() const { return cursor_; }
    [[nodiscard]] std::size_t size() const { return size_; }
    [[nodiscard]] std::size_t load_counter() const { return load_counter_; }

    [[nodiscard]] file_reader::line_iterator make_line_iterator(std::size_t start = 1);
    [[nodiscard]] file_reader::sentinel end() { return {}; }

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

        file_stream_.read(current_buffer_.data(), READER_BUFFER_SIZE);
        buffer_size_                  = file_stream_.gcount();
        current_buffer_[buffer_size_] = '\0';
    }

  private:
    std::filesystem::path file_path_; //!< path to the file to read

    std::ifstream file_stream_;       //!< file stream to read from
    std::string current_buffer_ {};   //!< buffer of the current read
    std::size_t buffer_size_ {0};     //!< size of the buffer
    std::size_t cursor_ {0};          //!< cursor in the buffer of the current block

    std::size_t size_ {0};            //!< file size in bytes
    std::size_t load_counter_ {0};    //!< counter to inform on how many load occurred
};

/**
 * @brief iterator through line of a read_file
 */
class file_reader::line_iterator {
  public:
    explicit line_iterator(file_reader* file_handler, std::size_t start_line_ = 1)
        : file_handler_(file_handler)
        , line_(file_handler->read_line(start_line_))
        , line_number_(start_line_) {}

    [[nodiscard]] std::size_t line() const { return line_number_; }

    const block_view& operator*() const { return line_; }
    const block_view* operator->() const { return &line_; }

    line_iterator& operator++() {
        if (*this == file_reader::sentinel {}) {
            return *this;
        }
        line_ = file_handler_->next_line();
        ++line_number_;
        return *this;
    }

    auto operator!=(const line_iterator& other) const {
        return (!line_.is_valid() || other.line_.is_valid()) && line_number_ != other.line_number_;
    }
    auto operator==(const line_iterator& other) const { return !(operator!=(other)); }
    bool operator!=(file_reader::sentinel) const {
        return file_handler_ == nullptr
            || (file_handler_->cursor_ < std::numeric_limits<decltype(file_handler_->get_buffer_cursor())>::max());
    }
    bool operator==(file_reader::sentinel s) const { return !(operator!=(s)); }

  private:
    file_reader* file_handler_;
    block_view line_ {};
    std::size_t line_number_ {0};
};

[[nodiscard]] inline file_reader::line_iterator file_reader::make_line_iterator(std::size_t start) {
    return file_reader::line_iterator(this, start);
}

/**
 * @brief wrapper around file_handler to manipulate lines per lines through a ranges interface
 */
class file_reader_line {
  public:
    explicit file_reader_line(file_reader& file_handler, std::size_t start_line_ = 1)
        : file_handler_(&file_handler)
        , start_line_(start_line_) {}

    file_reader::line_iterator begin() { return file_handler_->make_line_iterator(start_line_); }
    file_reader::sentinel end() { return file_handler_->end(); }

  private:
    file_reader* file_handler_;
    std::size_t start_line_;
};

} // namespace fil

#endif // FILE_H
