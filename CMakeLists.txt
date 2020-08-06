cmake_minimum_required(VERSION 3.6...3.15)
if (${CMAKE_VERSION} VERSION_LESS 3.15)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else ()
    cmake_policy(VERSION 3.15)
endif ()

project(fil CXX)

include(cmake/utility/BuiltFromSource.cmake)
include(CTest)

option(WITH_FIL_ROCKSDB "Compile with rocksdb" OFF)

if (IS_BUILT_FROM_SOURCE)
    include(cmake/utility/DoxygenSupport.cmake)

    set(WITH_FIL_ROCKSDB ON)
endif ()

find_package(fmt REQUIRED)

add_library(fil INTERFACE)
add_library(fys::fil ALIAS fil)
target_include_directories(fil INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/include/algorithm
        ${CMAKE_CURRENT_SOURCE_DIR}/include/datastructure
        ${CMAKE_CURRENT_SOURCE_DIR}/include/fsm
        ${CMAKE_CURRENT_SOURCE_DIR}/include/thread
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cli)
target_compile_features(fil INTERFACE cxx_std_20)
target_link_libraries(fil INTERFACE fmt::fmt)

add_subdirectory(internal/src/kv_db)

if (IS_BUILT_FROM_SOURCE AND BUILD_TESTING)
    message(STATUS "Tests will be built")
    add_subdirectory(tests)
endif ()
