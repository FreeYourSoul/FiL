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

#ifndef FIL_MEMBER_HH
#define FIL_MEMBER_HH

#include <functional>
#include <string>
#include <type_traits>
#include <utility>

#include "fil/meta/extract_member_type.hh"
#include "fil/meta/type_traits.hpp"

namespace fil::copa {
template<typename Ptr, bool IsSetter>
class member_fn {
  public:
    explicit member_fn(Ptr p)
        : ptr_(p) {}

    template<typename T, typename Value>
    constexpr void operator()(T& obj, Value&& value) {
        if constexpr (IsSetter) {
            obj.*ptr_(std::forward<Value>(value));
        } else {
            obj.*ptr_ = std::forward<Value>(value);
        }
    }

  private:
    Ptr ptr_;
};

template<auto MemberPtr = nullptr>
struct member {
    using pointer_member    = decltype(MemberPtr);
    using member_type       = extract_class_t<pointer_member>;
    using member_value_type = extract_value_type_t<pointer_member>;
    using is_member_ptr     = void;

    static constexpr bool is_function_member = std::is_member_function_pointer_v<pointer_member>;
    static constexpr bool is_vector          = is_std_vector<member_value_type>;

    template<typename Value>
    constexpr void operator()(member_type& obj, Value&& value) {
        if constexpr (is_function_member) {
            (obj.*MemberPtr)(std::forward<Value>(value));
        } else if constexpr (is_vector) {
            (obj.*MemberPtr).push_back(std::forward<Value>(value));
        } else {
            obj.*MemberPtr = std::forward<Value>(value);
        }
    }
};

template<auto Callback>
struct callback {
    constexpr auto operator()(auto&& value) const { //
        return Callback(std::forward<decltype(value)>(value));
    }
};

struct member_noop {
    using is_member_ptr     = void;
    using member_type       = int;
    using member_value_type = int;

    constexpr void operator()(auto&, auto&&) const {}
};

template<typename T>
concept member_type = requires {
    typename T::is_member_ptr;
    typename T::member_type;
    typename T::member_value_type;
};

template<typename T>
concept callback_type = std::is_invocable_v<T, std::string>;

template<typename T>
concept mem_or_cb_type = member_type<T> || callback_type<T>;

} // namespace fil::copa

#endif // FIL_MEMBER_HH
