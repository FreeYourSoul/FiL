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

#ifndef FIL_INCLUDE_FIL_KV_DB_KEY_VALUE_DB_HH
#define FIL_INCLUDE_FIL_KV_DB_KEY_VALUE_DB_HH

#include <vector>
#include <string>
#include <utility>

namespace fil {

using key_value = std::pair<std::string, std::string>;

template<typename DbPolicy>
class kv_db : DbPolicy {

 public:
   explicit kv_db(typename DbPolicy::initializer_type initializer) : DbPolicy(initializer) {}

   std::vector<key_value> multi_get(const std::vector<std::string>& keys) {
	  return DbPolicy::multi_get(keys);
   }

   key_value get(const std::string& key) {
	  return DbPolicy::multi_get(key);
   }

   bool set(const key_value& to_add) {
	  return DbPolicy::set(to_add);
   }

   bool multi_set(const std::vector<key_value>& to_adds) {
	  return DbPolicy::multi_set(to_adds);
   }
};

}// namespace fil

#endif//FIL_INCLUDE_FIL_KV_DB_KEY_VALUE_DB_HH
