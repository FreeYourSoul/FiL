cmake_minimum_required(VERSION 3.6...3.15)
if (${CMAKE_VERSION} VERSION_LESS 3.15)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else ()
    cmake_policy(VERSION 3.15)
endif ()

project(fil CXX)

# ---- Include guards ----
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()

find_package(fmt REQUIRED)
find_package(Catch2 QUIET)

include(cmake/utility/DoxygenSupport.cmake)

include(Catch)
include(CTest)

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

add_subdirectory(tests)