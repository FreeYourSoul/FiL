find_package(Poco COMPONENTS Net Util Foundation REQUIRED)

add_library(kamelia STATIC
        ${PROJECT_SOURCE_DIR}/include/fil/kamelia/node.hh
        ${PROJECT_SOURCE_DIR}/internal/src/kamelia/node.cpp
)
target_compile_features(kamelia INTERFACE cxx_std_23)
target_link_libraries(kamelia PUBLIC fys::fil Poco::Net Poco::Foundation Poco::Util)
target_include_directories(kamelia PRIVATE ${RocksDB_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/kv_db/include/)
add_library(fys::fil::kamelia ALIAS kamelia)
