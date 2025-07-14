cmake_minimum_required(VERSION 3.6...3.15)
if (${CMAKE_VERSION} VERSION_LESS 3.15)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else ()
    cmake_policy(VERSION 3.15)
endif ()

file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION version_str)
message(STATUS "version = ${version_str}")

project(fil VERSION ${version_str}
        DESCRIPTION "Fil is a set of libraries to provides quick and efficients tools not provided by the standard."
        LANGUAGES CXX)

include(misc/cmake/utility/BuiltFromSource.cmake)
include(CTest)

option(WITH_FIL_ROCKSDB "Compile with rocksdb" OFF)

if (IS_BUILT_FROM_SOURCE)
    include(misc/cmake/utility/DoxygenSupport.cmake)

    #    set(WITH_FIL_ROCKSDB ON)
endif ()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(fmt REQUIRED)

add_library(fil INTERFACE)
add_library(fys::fil ALIAS fil)
target_include_directories(fil INTERFACE
        $<BUILD_INTERFACE:
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/include/algorithm
        ${CMAKE_CURRENT_SOURCE_DIR}/include/datastructure
        ${CMAKE_CURRENT_SOURCE_DIR}/include/fsm
        ${CMAKE_CURRENT_SOURCE_DIR}/include/thread
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cli>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(fil INTERFACE cxx_std_23)
target_link_libraries(fil INTERFACE fmt::fmt)

add_subdirectory(internal/src/kv_db)

if (IS_BUILT_FROM_SOURCE AND BUILD_TESTING)
    message(STATUS "Tests will be built")
    add_subdirectory(tests)
endif ()


#
# installation
#
set(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Installation directory for libraries")
set(CMAKE_INSTALL_INCLUDEDIR include CACHE PATH "Installation directory for header files")

write_basic_package_version_file(
        "${PROJECT_BINARY_DIR}/filConfigVersion.cmake"
        VERSION "${version_str}"
        COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/misc/cmake/filConfig.cmake.in"
        "${PROJECT_BINARY_DIR}/filConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/fil"
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)
message("CMAKE_INSTALL_LIBDIR ----" ${CMAKE_INSTALL_LIBDIR})
message("CMAKE_INSTALL_BINDIR ----" ${CMAKE_INSTALL_BINDIR})
message("CMAKE_INSTALL_INCLUDEDIR ----" ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS fil
        EXPORT filTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(EXPORT filTargets
        FILE filTargets.cmake
        NAMESPACE fys::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/fil"
)

install(FILES
        "${PROJECT_BINARY_DIR}/filConfig.cmake"
        "${PROJECT_BINARY_DIR}/filConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/fil"
)

install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        FILES_MATCHING PATTERN "*.h*"
)


