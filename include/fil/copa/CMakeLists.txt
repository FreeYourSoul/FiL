cmake_minimum_required(VERSION 3.6...3.15)

add_library(copa INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/copa.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/debug_details.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/error.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/matcher.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/member.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/print_error.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/production.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/sink.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/rule.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/wrapper_utils.hh
)

add_library(fys::fil::copa ALIAS copa)

target_include_directories(copa INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(copa INTERFACE cxx_std_26)
target_link_libraries(copa INTERFACE fmt::fmt)



