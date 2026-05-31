include(GoogleTest)

# All test executables link cnc_core which brings in open62541 + math transitively

add_executable(test_telemetry_sim   test_telemetry_sim.cpp)
add_executable(test_information_model test_information_model.cpp)
add_executable(test_data_source     test_data_source.cpp)
add_executable(test_update_loop     test_update_loop.cpp)

foreach(t test_telemetry_sim test_information_model test_data_source test_update_loop)
    target_link_libraries(${t} PRIVATE cnc_core GTest::gtest_main)
    gtest_discover_tests(${t})
endforeach()
