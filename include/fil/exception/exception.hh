// MIT License
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

#ifndef FIL_INCLUDE_FIL_KV_DB_KV_DB_EXCEPTION_HH
#define FIL_INCLUDE_FIL_KV_DB_KV_DB_EXCEPTION_HH

#include <stdexcept>
#include <system_error>

#include <fmt/format.h>

namespace fil {

namespace except_cat {
struct db : public std::error_category {
   [[nodiscard]] const char* name() const noexcept override { return "fil::exception::db"; }
   [[nodiscard]] std::string message(int I) const override {
	  return fmt::format("Database Exception Category : {} : id {}", name(), I);
   }
};

struct fil : public std::error_category {
   [[nodiscard]] const char* name() const noexcept override { return "fil::exception"; }
};
}// namespace except_cat

class exception : public std::runtime_error {
 public:
   exception(const std::error_code& ec, const std::string& what) : std::runtime_error(what), _ec(ec) {}

   [[nodiscard]] std::uint64_t code() const { return _ec.value(); }

 private:
   std::error_code _ec;
};

}// namespace fil

#endif//FIL_INCLUDE_FIL_KV_DB_KV_DB_EXCEPTION_HH
