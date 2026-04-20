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

#ifndef FIL_TYPE_TRAITS_HH
#define FIL_TYPE_TRAITS_HH

#include <type_traits>
#include <vector>

namespace fil {

template<typename T>
concept is_std_vector = requires(T t) {
    typename T::value_type;
    typename T::allocator_type;
    requires std::same_as<T, std::vector<typename T::value_type, typename T::allocator_type>>;
};

template<typename T>
concept to_string_able = requires(const T& elem) {
    { std::to_string(elem) } -> std::convertible_to<std::string>;
} || requires(T elem) {
    { to_string(elem) } -> std::convertible_to<std::string>;
};

template<typename T>
concept is_iterator = !std::is_same_v<typename std::iterator_traits<T>::value_type, void>;

template<typename T, template<typename...> typename Template>
concept instance_of = requires { //
    []<template<typename...> typename U, typename... Args>(U<Args...>&) {}(std::declval<T&>());
};

} // namespace fil

#endif // FIL_TYPE_TRAITS_HH
