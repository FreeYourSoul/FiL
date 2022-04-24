# ---- Include guards ----
if (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif ()

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(IS_BUILT_FROM_SOURCE ON)
else()
    set(IS_BUILT_FROM_SOURCE OFF)
endif()
message(STATUS "code build from source ${IS_BUILT_FROM_SOURCE}")
