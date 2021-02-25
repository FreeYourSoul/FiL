// MIT License
//
// Copyright (c) 2020 Quentin Balland
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

#ifndef FIL_SUITABLE_HH
#define FIL_SUITABLE_HH

#include <type_traits>
#include <utility>

namespace fil {

namespace {
template<typename T, typename = void>
struct is_iterator {
   static constexpr bool value = false;
};

template<typename T>
struct is_iterator<T, std::enable_if_t<!std::is_same<typename std::iterator_traits<T>::value_type, void>::value>> {
   static constexpr bool value = true;
};

}// namespace

/**
 * @brief Get the most suitable (suitable being defined by a comparator) of a given container.
 * The most suitable follow the comparator.
 *
 * For example the most suitable following this comparator [](auto currentSuitable, auto next) { return currentSuitable < next; }
 * will be the biggest element of the container.
 *
 * @param first iterator start of the container
 * @param last iterator end of the container
 * @param comp predicate that takes the current most suitable and the next value to do a comparison
 * @param start is the input defining the default most suitable
 * @return iterator pointing on the maximum value described by the predicate, if container is empty, last is returned
 */
template<typename InputIt, typename ComparePredicate>
[[nodiscard]] InputIt find_most_suitable(InputIt first, InputIt last, ComparePredicate&& comp, InputIt start) {
   static_assert(is_iterator<InputIt>::value);

   auto suitable = start;
   while (first != last) {
	  if (std::forward<ComparePredicate>(comp)(*suitable, *first)) {
		 suitable = first;
	  }
	  ++first;
   }
   return suitable;
}

/**
 * @return the most suitable following a given comparator, the element defined by the iterator first is used as default most suitable
 */
template<typename InputIt, typename ComparePredicate>
[[nodiscard]] InputIt find_most_suitable(InputIt first, InputIt last, ComparePredicate&& comp) {
   return find_most_suitable(first, last, std::forward<ComparePredicate>(comp), first);
}

/**
 * Finding the most suitable over a container of container.
 * The first element of the first container is designed as default most suitable.
 */
template<typename InputIt, typename RetrieverLower, typename AlgorithmPredicate>
[[nodiscard]] InputIt compose_most_suitable(InputIt first, InputIt last, RetrieverLower&& retriever, AlgorithmPredicate&& algo) {
   static_assert(is_iterator<InputIt>::value);

   if (first == last) {
	  return first;
   }

   auto suitable = retriever(*first).begin();

   while (first != last) {
	  auto& inner_container = std::forward(retriever)(*first);
	  auto suitableTmp = std::forward(algo)(inner_container.begin(), inner_container.end(), suitable);
	  if (suitableTmp != inner_container.end()) {
		 suitable = suitableTmp;
	  }
	  ++first;
   }
   return suitable;
}

}// namespace fil
#endif//FIL_SUITABLE_HH
