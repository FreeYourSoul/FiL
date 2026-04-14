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
#include <string_view>
#include <utility>

#include "fil/meta/shallow_copy.hh"

namespace fil {

/**
 * max size of the buffer used to read the file (1Mb)
 * end position in the buffer to read from (-1 to have an extra byte for null-termination)
 */
static constexpr std::size_t READER_BUFFER_SIZE = 1024 * 1024 + 1;

/**
 * @brief Class responsible for reading and processing file data.
 *
 * The `file_reader` class provides functionality to read a file in blocks or lines, offering a buffered
 * view of its content for efficient processing. Additionally, it supports iterating through lines,
 * reading specific lines, or reading until a condition is met through predicates.
 */
class file_reader {
    template<typename>
    friend struct shallow_copy;

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
         * @return string view representing the block read from the file
         */
        [[nodiscard]] std::string_view get() const { return is_valid() ? block_ : std::string_view {}; }

      private:
        std::string_view block_ {};     //!< line retrieved from the buffer
        std::size_t load_id_block_ {0}; //!< load id that contains that block of text
        file_reader* reader_ {};        //!< pointer to the file reader that contains the line
    };

    template<std::invocable<std::string_view>>
    class iterator_file_ {};

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
        current_buffer_.resize(READER_BUFFER_SIZE);
        buffer_accessor_ = std::string_view(current_buffer_);
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

    /**
     * @brief Reads from the current buffer position until the predicate returns true for the accumulated string view.
     *
     * If the buffer doesn't have enough data remaining (less than minimum_size bytes), it triggers a buffer reload
     * before starting the read operation.
     *
     * @tparam Predicate A callable type that takes a std::string_view and returns a boolean
     * @param predicate A function or callable object that determines when to stop reading. It receives the accumulated
     *                  string_view from the start position up to the current cursor position.
     * @param minimum_size The minimum number of bytes that should remain in the buffer before triggering a reload (default: 100)
     * @return A block_view containing the characters read from the start position up to when the predicate returned true.
     *         Returns an empty block_view if the predicate never returns true before reaching the end of the buffer.
     */
    template<std::invocable<std::string_view> Predicate>
    [[nodiscard]] block_view read_until(Predicate&& predicate, std::size_t minimum_size = 100) {
        if (buffer_accessor_.front() != '\0' || ((buffer_size_ - cursor_) <= minimum_size)) {
            load_();
        }
        const auto start_cursor = cursor_;
        while (cursor_ < buffer_size_) {
            std::string_view sv = {buffer_accessor_.begin() + start_cursor, buffer_accessor_.begin() + ++cursor_};
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
            if (std::invoke(predicate, buffer_accessor_[++cursor_])) {
                return {
                    {buffer_accessor_.begin() + start_cursor, buffer_accessor_.begin() + ++cursor_},
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

        const auto pos       = std::find_if(std::next(buffer_accessor_.begin(), cursor_), buffer_accessor_.end(),
                                            [](auto c) { return c == '\n' || c == '\0'; });
        const auto size_line = std::distance(std::next(buffer_accessor_.begin(), cursor_), pos);

        std::string_view line {buffer_accessor_.data() + cursor_, static_cast<std::size_t>(size_line)};

        cursor_ = std::distance(buffer_accessor_.begin(), pos) + 1; // move past the newline character

        return {line, load_counter_, this};
    }

    [[nodiscard]] std::optional<std::uint8_t> next_byte() {
        if (cursor_ >= buffer_size_) {
            load_();
        }
        if (buffer_size_ == 0 || cursor_ >= buffer_size_) {
            return std::nullopt;
        }
        return std::make_optional(buffer_accessor_[cursor_++]);
    }

    [[nodiscard]] std::optional<std::uint8_t> previous_byte() {
        if ((cursor_ - 1) <= buffer_size_) {
            load_();
        }
        if (buffer_size_ == 0 || cursor_ <= 0) {
            return std::nullopt;
        }
        return std::make_optional(buffer_accessor_[--cursor_]);
    }

    [[nodiscard]] std::optional<std::uint8_t> peek() {
        if (cursor_ >= buffer_size_)
            return std::nullopt;
        return std::make_optional(buffer_accessor_[cursor_]);
    }

    [[nodiscard]] const std::filesystem::path& get_path() const { return file_path_; }
    [[nodiscard]] bool exists() const { return std::filesystem::exists(file_path_); }
    [[nodiscard]] auto get_file_cursor() { return file_stream_.tellg(); }
    [[nodiscard]] std::size_t get_buffer_cursor() const { return cursor_; }
    [[nodiscard]] std::size_t reader_cursor() const { return get_buffer_cursor(); }
    [[nodiscard]] std::size_t size() const { return size_; }
    [[nodiscard]] std::size_t load_counter() const { return load_counter_; }

    [[nodiscard]] file_reader::line_iterator make_line_iterator(std::size_t start = 1);
    [[nodiscard]] file_reader::sentinel end() { return {}; }

  private:
    file_reader() = default;

    /**
     * @brief Loads a block of data from the file into the buffer.
     *
     * @details This method reads a block of data from the file stream into the buffer and updates
     * internal state variables accordingly. If the buffer already contains unread data, the
     * cursor in the file stream is moved backward to account for the leftover data before loading.
     * It increments the load counter to track the number of load operations performed.
     *
     * Behavior:
     * - If the end of the file is reached, no actions are performed, and the method returns early.
     * - If the last read operation left unread data in the buffer, the file stream's cursor is
     *   adjusted to include that data in the next read operation.
     * - Resets the internal buffer state before attempting further reads.
     * - Reads up to `READER_BUFFER_SIZE` bytes from the file stream into the buffer. If no data can
     *   be read (e.g., due to an error or end-of-file), the buffer size remains at zero.
     * - Adds a null-terminator at the end of the loaded buffer for safe string operations.
     *
     * Preconditions:
     * - The file stream must be open and ready for reading.
     *
     * Postconditions:
     * - The buffer contains data read from the file, up to `READER_BUFFER_SIZE` bytes, or remains
     *   empty if no data could be read.
     * - The cursor is reset, and the buffer size is updated to reflect the amount of data read.
     */
    void load_() {
        if (file_stream_.eof())
            return;

        if (buffer_size_ != 0) {
            // move cursor backward to the last read position to retrieve the same leftover of buffer that was not read
            if (cursor_ < buffer_accessor_.size()) {
                file_stream_.seekg(-static_cast<int64_t>(buffer_accessor_.size() - cursor_), std::ios::cur);
            }
        }
        ++load_counter_;

        buffer_size_ = 0;
        cursor_      = 0;

        if (!file_stream_.good() || file_stream_.eof()) {
            return; // No more data to read
        }

        file_stream_.read(&current_buffer_[0], READER_BUFFER_SIZE);
        buffer_size_                  = file_stream_.gcount();
        current_buffer_[buffer_size_] = '\0';
        buffer_accessor_              = std::string_view(current_buffer_);
    }

  private:
    std::filesystem::path file_path_;     //!< path to the file to read

    std::ifstream file_stream_;           //!< file stream to read from
    std::string current_buffer_ {};       //!< buffer of the current read
    std::string_view buffer_accessor_ {}; //!< access point to the buffer
    std::size_t buffer_size_ {0};         //!< size of the buffer
    std::size_t cursor_ {0};              //!< cursor in the buffer of the current block

    std::size_t size_ {0};                //!< file size in bytes
    std::size_t load_counter_ {0};        //!< counter to inform on how many load occurred
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
 * @brief wrapper around file_handler to manipulate lines per lines through a range interface
 */
class file_reader_line {
  public:
    explicit file_reader_line(file_reader& file_handler, std::size_t start_line_ = 1)
        : file_handler_(&file_handler)
        , start_line_(start_line_) {}

    file_reader::line_iterator begin() const { return file_handler_->make_line_iterator(start_line_); }
    file_reader::sentinel end() const { return file_handler_->end(); }

  private:
    file_reader* file_handler_;
    std::size_t start_line_;
};

template<>
struct shallow_copy<file_reader> {
    static constexpr auto copy(file_reader& object) {
        file_reader shallow;

        shallow.file_stream_ = std::ifstream(object.file_path_);
        shallow.file_stream_.seekg(object.file_stream_.tellg());

        shallow.buffer_size_     = object.buffer_size_;
        shallow.cursor_          = object.cursor_;
        shallow.size_            = object.size_;
        shallow.file_path_       = object.file_path_;
        shallow.buffer_accessor_ = object.buffer_accessor_;
        shallow.load_counter_    = 0;
        return shallow;
    }

    static constexpr auto assign(file_reader& object, file_reader&& other) {
        object.file_stream_.seekg(other.file_stream_.tellg());
        object.cursor_ = other.cursor_;
        if (other.load_counter() > 0) {
            std::swap(object.current_buffer_, other.current_buffer_);
            object.buffer_accessor_ = std::string_view(object.current_buffer_);
        }
    }
};

} // namespace fil

#endif // FILE_H
