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

#ifndef FIL_STRING_HH
#define FIL_STRING_HH

#include <numeric>
#include <string>

namespace fil {

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

std::string join(const std::vector<std::string>& to_join, const std::string& separator = "") {
   auto size = separator.size() * to_join.size()
	   + std::accumulate(to_join.begin(), to_join.end(), 0, [](auto v, const auto& j) { return v + j.size(); });

   std::string result;
   result.reserve(size);

   bool first = true;
   for (const auto& j : to_join) {
	  if (first) {
		 first = false;
	  } else {
		 result.append(separator);
	  }
	  result.append(j);
   }
   return result;
}

}// namespace fil

#endif//FIL_STRING_HH
