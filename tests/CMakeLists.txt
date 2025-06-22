find_package(Catch2 3 REQUIRED)
include(Catch)

enable_testing()

add_executable(header_only_test
        ${CMAKE_CURRENT_SOURCE_DIR}/algorithm_testcase.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/fsm_testcase.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cli_testcase.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/boundary_map_testcase.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/rng_testcase.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/soa.cpp
        )
target_link_libraries(header_only_test fys::fil Catch2::Catch2WithMain)
catch_discover_tests(header_only_test)

if (WITH_FIL_ROCKSDB)
    add_executable(kv_db_rocksdb_test
            ${CMAKE_CURRENT_SOURCE_DIR}/kv_db_rocksdb_testcase.cpp
            rng_testcase.cpp)
    target_link_libraries(kv_db_rocksdb_test PUBLIC fys::fil::kv_db fys::fil Catch2::Catch2WithMain)
    target_link_libraries(kv_db_rocksdb_test PRIVATE Catch2::Catch2WithMain)
    catch_discover_tests(kv_db_rocksdb_test)
endif ()


