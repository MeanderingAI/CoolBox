add_library(robot_navigation STATIC
    source/robot_navigation.cpp
    source/controller_learning.cpp
    ../ML/tracker/source/extended_kalman_filter.cpp
    ../ML/tracker/source/unscented_kalman_filter.cpp
    ../ML/tracker/source/sequential_monte_carlo.cpp
    ../ML/multi_arm_bandit/source/bandit_agent.cpp
    ../ML/multi_arm_bandit/source/bandit_arm.cpp
    ../ML/multi_arm_bandit/source/ucb_agent.cpp
)
target_compile_features(robot_navigation PUBLIC cxx_std_17)
target_include_directories(robot_navigation PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/headers>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(robot_navigation PRIVATE ../ML/tracker/headers ../ML/multi_arm_bandit/headers)
target_link_libraries(robot_navigation PRIVATE mytrix)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    add_executable(robot_navigation_tests tests/test_robot_navigation.cpp)
    target_link_libraries(robot_navigation_tests PRIVATE robot_navigation tyst_framework_main)
    add_test(NAME RobotNavigationTests COMMAND robot_navigation_tests)
endif()