add_library(mars_regression SHARED
    source/mars_regression.cpp
)
target_include_directories(mars_regression PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(mars_regression PUBLIC generalized_linear_model mytrix)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(mars_regression_tests
        tests/test_mars_regression.cpp
    )
    target_link_libraries(mars_regression_tests PRIVATE mars_regression tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(mars_regression_tests)
    add_test(NAME MarsRegressionTests COMMAND mars_regression_tests)
endif()
