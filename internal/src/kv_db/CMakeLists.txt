if (WITH_FIL_ROCKSDB)

    find_package(RocksDB REQUIRED)

    add_library(kv_db STATIC
            ${PROJECT_SOURCE_DIR}/include/fil/kv_db/key_value_db.hh
            ${PROJECT_SOURCE_DIR}/include/fil/kv_db/kv_rocksdb.hh
            ${PROJECT_SOURCE_DIR}/internal/src/kv_db/kv_rocksdb.cpp
            )
    target_compile_features(kv_db INTERFACE cxx_std_20)
    target_link_libraries(kv_db PUBLIC fys::fil ${RocksDB_LIBRARIES})
    target_include_directories(kv_db PRIVATE ${RocksDB_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/kv_db/include/)
    add_library(fys::fil::kv_db ALIAS kv_db)

endif ()