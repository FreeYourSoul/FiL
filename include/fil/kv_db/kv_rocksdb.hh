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

#pragma once

#include <exception>
#include <memory>

#include "key_value_db.hh"

// forward declarations
namespace rocksdb {
class DB;
}
//! forward declarations

namespace fil {

class kv_rocksdb {

 public:
   static constexpr bool is_transactional = true;
   class transaction {
	public:
	  explicit transaction(kv_rocksdb& db);

	  std::string get(const std::string& key);

	  std::vector<std::string> multi_get(const std::vector<std::string>& keys);

	  template<typename T>
	  T get_as(const std::string& key) {
		 std::string value = get(key);
		 if constexpr (std::is_same_v<std::string, T>) {
			return value;
		 }
		 if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>) {
			return std::stoul(value);
		 }
		 if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
			return std::stoi(value);
		 }
		 if constexpr (std::is_floating_point_v<T>) {
			return std::stod(value);
		 }
		 throw std::logic_error("get_as not implemented");
	  }

	  bool set(const key_value& to_add);

	  bool multi_set(const std::vector<key_value>& to_add);

	private:
	  std::unique_ptr<rocksdb::Transaction> _transaction;

   };

   struct initializer_type {
	  std::string path_db_file;
	  std::vector<key_value> initial_kv;
   };

   explicit kv_rocksdb(const initializer_type& initializer);

 private:
   std::unique_ptr<rocksdb::OptimisticTransactionDB> _db;
};

}// namespace fil
