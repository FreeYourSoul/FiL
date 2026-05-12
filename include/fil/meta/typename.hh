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
#ifndef FIL_TYPENAME_HH
#define FIL_TYPENAME_HH

#include <array>
#include <string>
#include <string_view>
#include <utility>

namespace fil::meta {

template<std::size_t... Idxs>
constexpr auto substring_as_array(std::string_view str, std::index_sequence<Idxs...>) {
    return std::array {str[Idxs]..., '\n'};
}

template<typename T>
constexpr auto type_name_array() {
#if defined(__clang__)
    constexpr auto prefix   = std::string_view {"[T = "};
    constexpr auto suffix   = std::string_view {"]"};
    constexpr auto function = std::string_view {__PRETTY_FUNCTION__};
#elif defined(__GNUC__)
    constexpr auto prefix   = std::string_view {"with T = "};
    constexpr auto suffix   = std::string_view {"]"};
    constexpr auto function = std::string_view {__PRETTY_FUNCTION__};
#elif defined(_MSC_VER)
    constexpr auto prefix   = std::string_view {"type_name_array<"};
    constexpr auto suffix   = std::string_view {">(void)"};
    constexpr auto function = std::string_view {__FUNCSIG__};
#else
#error Unsupported compiler
#endif

    constexpr auto start = function.find(prefix) + prefix.size();
    constexpr auto end   = function.rfind(suffix);

    static_assert(start < end);

    constexpr auto name = function.substr(start, (end - start));
    return substring_as_array(name, std::make_index_sequence<name.size()> {});
}

template<typename T>
struct type_name_holder {
    static inline constexpr auto value = type_name_array<T>();
};

template<typename T>
constexpr auto type_name() -> std::string {
    constexpr auto& value = type_name_holder<T>::value;
    return std::string {value.data(), value.size()};
}

} // namespace fil::meta

#endif // FIL_TYPENAME_HH
