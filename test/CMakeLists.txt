enable_testing()

add_executable(fsm_test_bin
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/state_machine_testcase.cpp)
target_link_libraries(fsm_test_bin fil)
catch_discover_tests(fsm_test_bin)

add_executable(cli_test_bin
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cli_testcase.cpp)
target_link_libraries(cli_test_bin fil)
catch_discover_tests(cli_test_bin)

