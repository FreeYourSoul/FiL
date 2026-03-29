cmake_minimum_required(VERSION 3.6...3.15)

add_library(descpa INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/copa.hh)

add_library(fys::fil::descpa ALIAS descpa)

target_include_directories(descpa INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/fil/descpa/descpa.hh>
)

target_compile_features(descpa INTERFACE cxx_std_26)
target_link_libraries(descpa INTERFACE fmt::fmt)



