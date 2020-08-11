#include <rocksdb/options.h>
#include <rocksdb/utilities/optimistic_transaction_db.h>
#include <rocksdb/utilities/transaction.h>

#include <fmt/format.h>

#include <fil/exception/exception.hh>
#include <fil/kv_db/kv_rocksdb.hh>

namespace fil {

kv_rocksdb::transaction::transaction(kv_rocksdb& db) {
   rocksdb::WriteOptions write_options;
   rocksdb::Transaction* txn = db._db->BeginTransaction(write_options);
   txn->SetSnapshot();
   _transaction.reset(txn);
}

std::string kv_rocksdb::transaction::get(const std::string& key) {
   std::string result;
   rocksdb::ReadOptions opt;

   auto s = _transaction->Get(opt, key, &result);
   if (!s.ok()) {
	  throw fil::exception(get_error_code(), fmt::format("Error while getting key {} : {}", key, s.ToString()));
   }
   return result;
}

std::vector<std::string> kv_rocksdb::transaction::multi_get(const std::vector<std::string>& keys) {
   std::vector<std::string> results;
   rocksdb::ReadOptions opt;
   std::vector<rocksdb::Slice> slicing;

   slicing.reserve(keys.size());
   for (const auto& key : keys) {
	  slicing.emplace_back(key);
   }
   auto status = _transaction->MultiGet(opt, slicing, &results);

   for (const auto& s : status) {
	  if (!s.ok()) {
		 throw fil::exception(get_error_code(), fmt::format("Error while multi getting {} values : {}", keys.size(), s.ToString()));
	  }
   }
   return results;
}

bool kv_rocksdb::transaction::set(const key_value& to_add) {
   std::string value;
   rocksdb::Status s = _transaction->Put(to_add.first, to_add.second);
   if (!s.ok()) {
	  throw fil::exception(put_error_code(), fmt::format("Error while inserting key-value : {}-{} : {}", to_add.first, to_add.second, s.ToString()));
   }
   return true;
}

bool kv_rocksdb::transaction::multi_set(const std::vector<key_value>& to_adds) {
   for (const auto& to_add : to_adds) {
	  set(to_add);
   }
   return true;
}

bool kv_rocksdb::transaction::commit_transaction() {
   auto s = _transaction->Commit();
   if (!s.ok()) {
	  throw fil::exception(commit_error_code(), fmt::format("Commit Failure : {}", s.ToString()));
   }
   return true;
}

kv_rocksdb::kv_rocksdb(const initializer_type& initializer) {

   rocksdb::OptimisticTransactionDB* txn_db;
   rocksdb::Options option = []() {
				rocksdb::Options opt;
				opt.create_if_missing = true;
				return opt; }();

   auto status = rocksdb::OptimisticTransactionDB::Open(option, initializer.path_db_file, &txn_db);

   if (!status.ok()) {
	  throw std::runtime_error(fmt::format(FMT_STRING("Couldn't open optimistic RocksDB Database: {}"), status.ToString()));
   }

   _db.reset(txn_db);
}

}// namespace fil
