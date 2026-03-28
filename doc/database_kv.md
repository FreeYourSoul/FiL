# Database KV

FiL provides a Key-Value datastore abstraction. This part of the library is NOT header-only and requires compilation
with specific options to enable different backends.

## Supported Policies

Currently, the following policies are supported (or planned):

- **RocksDB**: Fast and reliable KV engine by Meta.
- **Redis**: Persistent KV in-memory database.
- **Couchbase**: NoSQL performant database.

These datastores are selected because they can be used in-memory, making them excellent choices for local or small-scale
projects.

## Basic Usage

The abstraction uses a policy-based design. You instantiate a `kv_db` with the desired backend policy.

```cpp
#include <fil/kv_db/key_value_db.hh>
#include <fil/kv_db/kv_rocksdb.hh> // If using RocksDB

// Example with RocksDB policy
// Note: requires WITH_FIL_ROCKSDB CMake option
fil::rocksdb_initializer init;
init.path = "/tmp/testdb";
fil::kv_db<fil::kv_rocksdb_policy> db(init);

// Basic operations
db.set({"key", "value"});
auto values = db.get("key");

// Multi-get
auto kvs = db.multi_get({"key1", "key2"});

// Iteration
db.list("prefix", [](std::string_view key, std::string_view value) {
    std::cout << key << ": " << value << std::endl;
    return true; // continue iteration
});
```

## Transactional Support

Some policies support transactions.

```cpp
auto tx = db.make_transaction();
tx->set({"key", "value"});
tx->commit();
```

## Compilation Options

To use specific backends, enable them in your CMake configuration:

- `WITH_FIL_ROCKSDB`: Enables RocksDB backend.
- `WITH_FIL_REDIS`: Enables Redis backend.
- `WITH_FIL_COUCHBASE`: Enables Couchbase backend.
