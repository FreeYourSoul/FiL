// MIT License
//
// Copyright (c) 2020 Quentin Balland
// Repository : https://github.com/FreeYourSoul/FyS
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

#ifndef FIL_CONTAINS_HH
#define FIL_CONTAINS_HH

#include <algorithm>
#include <vector>

namespace fil {

/**
 * Check if all element of a vector are present in another one (using an accessor to retrieve the other one element T)
 *
 * @tparam T type of the first vector (to check type)
 * @tparam R type of the second vector (type checked)
 * @tparam Accessor accessor function used in order to retrieve a type to check T from the container containing type R
 * This function has to take a to check type as parameter (type T) and return a type checked type (R)
 *
 * @param to_check vector to check if the content is contained in another vector
 * @param container vector to check against
 * @param accessor take the accessor function
 * @return true if all the element from toCheck are in container following the accessor, false otherwise
 */
template<typename T, typename R, typename Accessor>
[[nodiscard]] bool all_contains(const std::vector<T>& to_check, const std::vector<R>& container, Accessor&& accessor) {
   return std::all_of(to_check.cbegin(), to_check.cend(), [container, &accessor](const T& lhs) {
	  return std::find_if(container.cbegin(), container.cend(),
						  [&lhs, &accessor](const auto& v) { return lhs == accessor(v); })
		  != container.cend();
   });
}

/**
 * Check if all element of a vector are present in another one (with an accessor for both vector)
 *
 * @tparam T type of the vector to check (using the accessor to do the check)
 * @tparam Accessor accessor function used in order to retrieve a type to check from the containers
 *
 * @param to_check vector to check if the content is contained in another vector
 * @param container vector to check against
 * @param accessor take the accessor function
 * @return true if all the element from toCheck are in container following the accessor, false otherwise
 */
template<typename T, typename Accessor>
[[nodiscard]] bool all_contains(const std::vector<T>& to_check, const std::vector<T>& container, Accessor&& accessor) {
   return std::all_of(to_check.cbegin(), to_check.cend(), [container, &accessor](const T& lhs) {
	 return std::find_if(container.cbegin(), container.cend(),
						 [&lhs, &accessor](const auto& v) { return accessor(lhs) == accessor(v); })
		 != container.cend();
   });
}


template<typename T>
[[nodiscard]] bool all_contains(const std::vector<T>& to_check, const std::vector<T>& container) {
   return std::all_of(to_check.cbegin(), to_check.cend(), [container](const T& elem) {
	  return std::find(container.cbegin(), container.cend(), elem) != container.cend();
   });
}

}// namespace fil

#endif//FIL_CONTAINS_HH
