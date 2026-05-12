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

#ifndef FIL_STRING_HH
#define FIL_STRING_HH

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

#include "fil/meta/type_traits.hpp"

namespace fil {

template<typename T>
[[nodiscard]] std::string to_string(const T& elems) = delete ("A specialization of fil::to_string for the type must be provided for type");

template<typename T>
requires requires(T d) {
    requires std::is_integral_v<std::decay_t<T>> || std::is_floating_point_v<std::decay_t<T>>;
    { std::to_string(d) } -> std::convertible_to<std::string>;
}
[[nodiscard]] std::string to_string(const T& elem) {
    return std::to_string(elem);
}

template<>
[[nodiscard]] inline std::string to_string(const std::string& elem) {
    return elem;
}

template<>
[[nodiscard]] inline std::string to_string(const std::string_view& elem) {
    return std::string {elem};
}

template<typename T>
requires requires(T d) {
    { d.to_string() } -> std::convertible_to<std::string>;
}
[[nodiscard]] std::string to_string(const T& elem) {
    return elem.to_string();
}

template<typename T>
requires requires(T d) {
    requires(std::is_array_v<T> && std::is_same_v<std::remove_extent_t<T>, char>); //
}
[[nodiscard]] std::string to_string(const T& elem) {
    return std::string(elem);
}

template<typename T>
requires requires(T d) {
    requires std::ranges::range<T>;
    requires !(std::is_array_v<T> && std::is_same_v<std::remove_extent_t<T>, char>);
    requires !std::is_same_v<std::decay_t<T>, std::string>;
    requires !std::is_same_v<std::decay_t<T>, std::string_view>;
}
[[nodiscard]] std::string to_string(const T& elems) {
    return std::ranges::fold_left( //
               elems, std::string {"["},
               [start = true](std::string acc, auto&& elem) mutable {
                   if (!start)
                       acc += ", ";
                   start = false;
                   return acc + to_string(elem);
               })
         + "]";
}

template<typename T>
concept is_stringifiable = requires(const T& elem) {
    { fil::to_string(elem) } -> std::convertible_to<std::string>;
};

template<typename Handler>
void split_string(const std::string& input, const std::string& separator, Handler&& handler, int limitation = -1) {

    std::size_t pos_start = 0;
    std::size_t pos_end;

    do {
        if (limitation != -1 && --limitation <= 0) {
            pos_end = std::string::npos;
        } else {
            pos_end = input.find(separator, pos_start);
        }
        std::forward<Handler>(handler)(input.substr(pos_start, pos_end - pos_start));
        pos_start = pos_end + separator.size();
    } while (pos_end != std::string::npos);
}

template<typename Handler>
void split_string(const std::string& input, const std::vector<std::string>& separators, Handler&& handler, int limitation = -1) {

    std::size_t pos_start = 0;
    std::size_t pos_end;
    std::vector<std::size_t> tmp;

    tmp.resize(separators.size());
    do {
        for (std::uint32_t i = 0; i < separators.size(); ++i) {
            auto v = input.find(separators.at(i), pos_start);
            tmp[i] = (v == std::string::npos) ? input.size() : v;
        }
        std::size_t index = std::distance(tmp.begin(), std::min_element(tmp.begin(), tmp.end()));
        if (limitation != -1 && --limitation <= 0) {
            pos_end = input.size();
        } else {
            pos_end = tmp[index];
        }
        std::forward<Handler>(handler)(input.substr(pos_start, pos_end - pos_start));
        pos_start = pos_end + separators[index].size();

    } while (pos_end < input.size());
}

template<fil::is_stringifiable Element>
std::string join(const std::vector<Element>& to_join, const std::string& separator = "") {
    return std::ranges::fold_left(to_join, std::string {}, [&separator, start = true](auto acc, auto&& elem) mutable {
        if (!start)
            acc += separator;
        start = false;
        return acc + fil::to_string(elem);
    });
}

inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::ranges::find_if(s, [](auto ch) { return !std::isspace(ch); }));
}

inline void rtrim(std::string& s) {
    s.erase(std::ranges::find_if(s.rbegin(), s.rend(), [](auto ch) { return !std::isspace(ch); }).base(), s.end());
}

} // namespace fil

#endif // FIL_STRING_HH
