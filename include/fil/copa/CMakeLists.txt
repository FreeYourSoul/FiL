cmake_minimum_required(VERSION 3.6...3.15)

add_library(copa INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/copa.hh)

add_library(fys::fil::copa ALIAS copa)

target_include_directories(copa INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(copa INTERFACE cxx_std_26)
target_link_libraries(copa INTERFACE fmt::fmt)



