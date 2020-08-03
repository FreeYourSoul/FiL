cmake_minimum_required(VERSION 3.6...3.15)
if (${CMAKE_VERSION} VERSION_LESS 3.15)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else ()
    cmake_policy(VERSION 3.15)
endif ()

project(fil CXX)

# ---- Include guards ----
if (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif ()

find_package(fmt REQUIRED)

include(cmake/utility/DoxygenSupport.cmake)

add_library(fil INTERFACE)
add_library(fys::fil ALIAS fil)
target_include_directories(fil INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/include/algorithm
        ${CMAKE_CURRENT_SOURCE_DIR}/include/datastructure
        ${CMAKE_CURRENT_SOURCE_DIR}/include/fsm
        ${CMAKE_CURRENT_SOURCE_DIR}/include/thread
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cli)
target_compile_features(fil INTERFACE cxx_std_17)
target_link_libraries(fil INTERFACE fmt::fmt)

add_library(kv_db STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/fil/kv_db/key_value_db.hh
        )
add_library(fys::fil::kv_db ALIAS kv_db)
target_compile_features(kv_db INTERFACE cxx_std_20)
target_link_libraries(kv_db PUBLIC fys::fil)
target_include_directories(kv_db PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/kv_db/include/)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND BUILD_TESTING)
    find_package(Catch2 QUIET)
    include(Catch)
    include(CTest)

    add_subdirectory(tests)
endif ()
