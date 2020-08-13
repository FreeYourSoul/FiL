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

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace fil {

using key_value = std::pair<std::string, std::string>;

template<typename DbPolicy, class Enable = void>
class kv_db : public DbPolicy {};

// ******* Non-Transactional part *******

// TODO : Enforce that db has the method they should have with concept
//        It should also remove the SFINAE code and become specialization of the DbPolicy
template<typename DbPolicy>
class kv_db<DbPolicy, std::enable_if_t<!DbPolicy::is_transactional>> : public DbPolicy {

 public:
   explicit kv_db(const typename DbPolicy::initializer_type& initializer) : DbPolicy(initializer) {}

   std::vector<std::string> get(const std::string& key) {
	  return DbPolicy::multi_get(key);
   }

   std::vector<key_value> multi_get(const std::vector<std::string>& keys) {
	  return DbPolicy::multi_get(keys);
   }

   bool set(const key_value& to_add) {
	  return DbPolicy::set(to_add);
   }

   bool multi_set(const std::vector<key_value>& to_adds) {
	  return DbPolicy::multi_set(to_adds);
   }

   void inc_counter(const std::string& key_counter) {
	  return DbPolicy::inc_counter(key_counter);
   }

   template<typename T>
   T get_as(const std::string& key) {
	  return DbPolicy::template get_as<T>(key);
   }
};

// ******* Transactional part *******


// TODO : Enforce that Transaction has the method they should have with concept
//        It should also remove the SFINAE code and become specialization of the DbPolicy
template<typename DbPolicy>
class kv_db<DbPolicy, std::enable_if_t<DbPolicy::is_transactional>> : public DbPolicy {
 public:
   using transaction_type = typename DbPolicy::transaction;

   explicit kv_db(const typename DbPolicy::initializer_type& initializer) : DbPolicy(initializer) {}

   std::unique_ptr<transaction_type> make_transaction() {
	  return std::make_unique<transaction_type>(*this);
   }
};

}// namespace fil

#endif//FIL_INCLUDE_FIL_KV_DB_KEY_VALUE_DB_HH
