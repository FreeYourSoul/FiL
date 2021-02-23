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

#ifndef FIL_INCLUDE_FIL_DATASTRUCTURE_FIXED_CIRCULAR_BUFFER_HH
#define FIL_INCLUDE_FIL_DATASTRUCTURE_FIXED_CIRCULAR_BUFFER_HH

#include <array>

namespace fil {

template <typename T, unsigned SIZE>
class fixed_circular_buffer {

 public:
   fixed_circular_buffer(std::optional<T> fill_with) {
	  if (fill_with.has_value()) {
	  	std::fill(_buffer.begin(), _buffer.end(), fill_with.value());
	  }
   }

   void push(T&& to_push) {
	  _buffer[_index] = std::forward<T>(to_push);
	  _index = (_index + 1) % SIZE;
   }

 private:
   std::array<T, SIZE> _buffer{};

   std::uint32_t _index = 0;
};

}

#endif//FIL_INCLUDE_FIL_DATASTRUCTURE_FIXED_CIRCULAR_BUFFER_HH
