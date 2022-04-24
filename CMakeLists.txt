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

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# GNUInstallDirs "DATADIR" wrong here; CMake search path wants "share".
set(FIL_CMAKECONFIG_INSTALL_DIR "share/cmake/fil" CACHE STRING "install path for FilConfig.cmake")

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
  $<INSTALL_INTERFACE:include>)
      
target_compile_features(fil INTERFACE cxx_std_20)
target_link_libraries(fil INTERFACE fmt::fmt)

add_subdirectory(internal/src/kv_db)

file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION version_str)
message(STATUS "version = ${version_str}")

write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/filConfigVersion.cmake"
    VERSION "${version_str}"
    COMPATIBILITY AnyNewerVersion
    )
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/filConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/filConfig.cmake"
    INSTALL_DESTINATION ${FIL_CMAKECONFIG_INSTALL_DIR}
    )
  
install(TARGETS fil
  EXPORT filTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES ${PROJECT_SOURCE_DIR}/include/fil
  PUBLIC_HEADER DESTINATION include COMPONENT Development
  BUNDLE DESTINATION bin COMPONENT Runtime
)
install(EXPORT filTargets NAMESPACE fys:: DESTINATION ${FIL_CMAKECONFIG_INSTALL_DIR})
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/filConfigVersion.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/filConfig.cmake"
    DESTINATION ${FIL_CMAKECONFIG_INSTALL_DIR})
  
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include)
      
if (IS_BUILT_FROM_SOURCE AND BUILD_TESTING)
    message(STATUS "Tests will be built")
    add_subdirectory(tests)
endif ()
