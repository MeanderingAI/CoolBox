add_library(tracker SHARED
    source/extended_kalman_filter.cpp
    source/kalman_filter.cpp
    source/sequential_monte_carlo.cpp
    source/unscented_kalman_filter.cpp
)
target_include_directories(tracker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Tests
enable_testing()
add_executable(tracker_tests
    tests/test_extended_kalman_filter.cpp
    tests/test_kalman_filter.cpp
    tests/test_sequential_monte_carlo.cpp
    tests/test_unscented_kalman_filter.cpp
)
target_link_libraries(tracker_tests PRIVATE tracker GTest::gtest_main)
add_test(NAME TrackerTests COMMAND tracker_tests)
