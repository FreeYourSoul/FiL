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

#ifndef FIL_COPA_ERROR_HH
#define FIL_COPA_ERROR_HH

#include <any>
#include <string>
#include <vector>

namespace fil::copa {

struct error_parsing {
    std::string token_failure; //!< token on which the error occurred
    std::size_t cursor;        //!< cursor at which the error occurred
    std::string parsing_step;  //!< name of the matcher failing
    std::string error_brief;   //!< brief error
};

class error_stack {
  public:
    using difference_type   = std::ptrdiff_t;
    using value_type        = error_parsing;
    using pointer           = error_parsing*;
    using reference         = error_parsing&;
    using iterator_category = std::random_access_iterator_tag;

    error_stack() = default;
    explicit error_stack(error_parsing&& error) { push(std::move(error)); }

    void push(error_parsing&& error) { stack.push_back(std::move(error)); }
    void clear() { stack.clear(); }
    void pretty_print_error() const;

    [[nodiscard]] std::size_t size() const { return stack.size(); }

    std::vector<error_parsing>::iterator begin() { return stack.begin(); }
    std::vector<error_parsing>::iterator end() { return stack.end(); }
    std::vector<error_parsing>::reverse_iterator rbegin() { return stack.rbegin(); }
    std::vector<error_parsing>::reverse_iterator rend() { return stack.rend(); }

  private:
    std::vector<error_parsing> stack;
};

} // namespace fil::copa

#endif // FIL_COPA_ERROR_HH
