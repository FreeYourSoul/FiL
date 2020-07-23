enable_testing()

add_executable(fsm_test_bin
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/fsm_testcase.cpp)
target_link_libraries(fsm_test_bin fil)
catch_discover_tests(fsm_test_bin)

add_executable(cli_test_bin
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cli_testcase.cpp)
target_link_libraries(cli_test_bin fil)
catch_discover_tests(cli_test_bin)

add_executable(command_test_bin
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        )
target_link_libraries(command_test_bin fil)
catch_discover_tests(command_test_bin)

add_executable(datastructure_test_bin
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/boundary_map_testcase.cpp
        )
target_link_libraries(datastructure_test_bin fil)
catch_discover_tests(datastructure_test_bin)

add_executable(algorithm_test_bin
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/algorithm_testcase.cpp
        )
target_link_libraries(algorithm_test_bin fil)
catch_discover_tests(algorithm_test_bin)


