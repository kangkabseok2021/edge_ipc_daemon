add_executable(edge_ipc_tests
    test_filters.cpp
    test_sources.cpp
    test_spsc.cpp
    test_fsm.cpp
)
target_link_libraries(edge_ipc_tests PRIVATE edge_ipc_core GTest::gtest_main)
target_compile_options(edge_ipc_tests PRIVATE -Wall -Wextra)

include(GoogleTest)
gtest_discover_tests(edge_ipc_tests)
