cmake_minimum_required(VERSION 3.6...3.15)
if (${CMAKE_VERSION} VERSION_LESS 3.15)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else ()
    cmake_policy(VERSION 3.15)
endif ()

project(fil CXX)

find_package(Catch2 REQUIRED)
include(Catch)
include(CTest)

add_library(fys::fil INTERFACE)

target_include_directories(fil INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/include/fsm
        ${CMAKE_CURRENT_SOURCE_DIR}/include/thread
        ${CMAKE_CURRENT_SOURCE_DIR}/include/opt)

enable_testing()

add_executable(fsm_test_bin
        ${CMAKE_CURRENT_SOURCE_DIR}/test/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/state_machine_testcase.cpp)
target_link_libraries(fsm_test_bin fil)
catch_discover_tests(fsm_test_bin)
