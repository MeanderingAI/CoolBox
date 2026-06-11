add_library(dynamic_bayesian_network SHARED
    source/dynamic_bayesian_network.cpp
)
target_include_directories(dynamic_bayesian_network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(dynamic_bayesian_network_tests
        tests/test_dynamic_bayesian_network.cpp
    )
    target_link_libraries(dynamic_bayesian_network_tests PRIVATE dynamic_bayesian_network tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(dynamic_bayesian_network_tests)
    add_test(NAME DynamicBayesianNetworkTests COMMAND dynamic_bayesian_network_tests)
endif()
