#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/optimistic_transaction_db.h>

#include <fmt/format.h>

#include <fil/kv_db/kv_rocksdb.hh>

namespace fil {

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

	_db.reset(txn_db->GetBaseDB());
  }

}
