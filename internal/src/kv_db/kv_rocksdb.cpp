#include <rocksdb/options.h>
#include <rocksdb/utilities/transaction.h>
#include <rocksdb/utilities/optimistic_transaction_db.h>

#include <fmt/format.h>

#include <fil/kv_db/kv_rocksdb.hh>

namespace fil {

kv_rocksdb::transaction::transaction(kv_rocksdb& db) {
   rocksdb::WriteOptions write_options;
   rocksdb::Transaction* txn = db._db->BeginTransaction(write_options);
   txn->SetSnapshot();
   _transaction.reset(txn);
}

std::string kv_rocksdb::transaction::get(const std::string& key) {

}


bool kv_rocksdb::transaction::set(const key_value& to_add) {

}

bool kv_rocksdb::transaction::multi_set(const std::vector<key_value>& to_add) {

}

std::vector<std::string> kv_rocksdb::transaction::multi_get(const std::vector<std::string>& keys) {

}

kv_rocksdb::kv_rocksdb(const initializer_type& initializer) {

    rocksdb::OptimisticTransactionDB* txn_db;
    rocksdb::Options option = []() {
				rocksdb::Options opt;
				opt.create_if_missing = true;
				return opt; }();
    
    rocksdb::Status status = rocksdb::OptimisticTransactionDB::Open(option, initializer.path_db_file, &txn_db);

    if (!status.ok()) {
      throw std::runtime_error(fmt::format(FMT_STRING("Couldn't open optimistic RocksDB Database: {}"), status.ToString()));
    }

	_db.reset(txn_db);
  }

}
