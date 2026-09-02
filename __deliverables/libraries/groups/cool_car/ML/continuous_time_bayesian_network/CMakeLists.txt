add_library(continuous_time_bayesian_network SHARED
    source/continuous_time_bayesian_network.cpp
)
target_include_directories(continuous_time_bayesian_network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(continuous_time_bayesian_network_tests
        tests/test_continuous_time_bayesian_network.cpp
    )
    target_link_libraries(continuous_time_bayesian_network_tests PRIVATE continuous_time_bayesian_network tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(continuous_time_bayesian_network_tests)
    add_test(NAME ContinuousTimeBayesianNetworkTests COMMAND continuous_time_bayesian_network_tests)
endif()
