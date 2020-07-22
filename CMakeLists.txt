cmake_minimum_required(VERSION 3.6...3.15)
if (${CMAKE_VERSION} VERSION_LESS 3.15)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else ()
    cmake_policy(VERSION 3.15)
endif ()

project(fil CXX)

find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)

include(Catch)
include(CTest)

add_library(fil INTERFACE)
add_library(fys::fil ALIAS fil)
target_include_directories(fil INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/include/fsm
        ${CMAKE_CURRENT_SOURCE_DIR}/include/thread
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cli)
target_link_libraries(fil INTERFACE fmt::fmt)

add_subdirectory(test)
