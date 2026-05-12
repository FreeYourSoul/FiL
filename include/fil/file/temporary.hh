/// MIT License
//
// Copyright (c) 2025 Quentin Balland
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

#ifndef FIL_TEMPORARY_HH
#define FIL_TEMPORARY_HH

#include <filesystem>
#include <format>
#include <random>

namespace fil {

/**
 * @brief Create a temporary file with a unique generated name on the temporary folder of the current OS.
 * This file is removed upon destruction of the object
 */
class temporary_file {
  public:
    explicit temporary_file(const std::string& content, const std::string& prefix = "temp_file", const std::string& extension = ".txt")
        : path_([&] {
            // Generate a unique filename
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(1000, 9999);

            const auto filename  = std::format("{}_{}{}", prefix, dis(gen), extension);
            const auto temp_path = std::filesystem::temp_directory_path() / filename;
            std::ofstream file(temp_path);
            if (!file) {
                throw std::runtime_error(std::format("Could not open file for writing: {}", temp_path.string()));
            }

            file << content;
            if (!file) {
                throw std::runtime_error(std::format("Error writing to file: {}", temp_path.string()));
            }

            return temp_path;
        }()) {}

    ~temporary_file() { std::filesystem::remove(path_); }

    operator std::filesystem::path() const { return path_; }

  private:
    std::filesystem::path path_;
};
} // namespace fil

#endif // FIL_TEMPORARY_HH
