add_library(tracker SHARED
    source/extended_kalman_filter.cpp
    source/kalman_filter.cpp
    source/mobile_robot_model_calibration.cpp
    source/sequential_monte_carlo.cpp
    source/social_group_handover.cpp
    source/unscented_kalman_filter.cpp
)
target_include_directories(tracker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(tracker PUBLIC mytrix)

# Tests
if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(tracker_tests
        tests/test_extended_kalman_filter.cpp
        tests/test_kalman_filter.cpp
        tests/test_mobile_robot_model_calibration.cpp
        tests/test_sequential_monte_carlo.cpp
        tests/test_social_group_handover.cpp
        tests/test_unscented_kalman_filter.cpp
    )
    target_link_libraries(tracker_tests PRIVATE tracker tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(tracker_tests)
    add_test(NAME TrackerTests COMMAND tracker_tests)
endif()
