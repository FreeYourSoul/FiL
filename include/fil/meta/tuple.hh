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

#ifndef FIL_TUPLE_HH
#define FIL_TUPLE_HH

#include <tuple>

namespace fil {

/**
 * @brief Extracts elements from a tuple based on their types.
 *
 * @details Filters a tuple and creates a new tuple containing only the elements
 * whose types match one or more of the specified target types. The relative order
 * of matched elements is preserved in the result.
 *
 * @tparam Targets The target types to extract from the input tuple. All elements in
 *                  the input tuple that have a type matching any of these targets
 *                  will be included in the result. Can be one or more types.
 *
 * @tparam Args The element types of the input tuple. This is automatically deduced
 *              from the tuple argument and should not be explicitly specified.
 *
 * @param t The input tuple from which elements will be extracted. The tuple is passed
 *          by const reference and is not modified.
 *
 * @return A new @c std::tuple containing the extracted elements in the same order
 *         as they appeared in the input tuple. If no elements match any target type,
 *         an empty tuple is returned. The return type is automatically deduced.
 *
 * @note The type matching is exact; implicit conversions are not considered.
 *
 * @note All filtering is performed at compile-time. The resulting tuple has a concrete,
 *       well-defined type determined during compilation.
 */
template<typename... Targets, typename... Args>
auto extract_tuple(const std::tuple<Args...>& t) {
    auto helper = []<std::size_t I>(std::integral_constant<std::size_t, I>, const std::tuple<Args...>& tuple) {
        using element_type = std::tuple_element_t<I, std::tuple<Args...>>;
        if constexpr ((std::is_same_v<element_type, Targets> || ...)) {
            return std::make_tuple(std::get<I>(tuple));
        } else {
            return std::tuple<>();
        }
    };

    return std::tuple_cat([&]<std::size_t... I>(std::index_sequence<I...>) {
        return std::tuple_cat(helper(std::integral_constant<std::size_t, I> {}, t)...);
    }(std::index_sequence_for<Args...> {}));
}

} // namespace fil

#endif // FIL_TUPLE_HH
