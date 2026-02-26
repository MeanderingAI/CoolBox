add_library(marked_point_process SHARED
    source/marked_point_process.cpp
    source/piecewise_conditional_intensity_model.cpp
)
target_include_directories(marked_point_process PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(marked_point_process PRIVATE Eigen3::Eigen)

# Tests
enable_testing()
add_executable(marked_point_process_tests
    tests/test_marked_point_process.cpp
    tests/test_piecewise_conditional_intensity_model.cpp
)
target_link_libraries(marked_point_process_tests PRIVATE marked_point_process GTest::gtest_main)
add_test(NAME MarkedPointProcessTests COMMAND marked_point_process_tests)
