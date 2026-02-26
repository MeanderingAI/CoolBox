add_library(bayesian_network SHARED
    source/bayesian_network.cpp
)
target_include_directories(bayesian_network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Tests
enable_testing()
add_executable(bayesian_network_tests
    tests/test_baysian_network.cpp
)
target_link_libraries(bayesian_network_tests PRIVATE bayesian_network GTest::gtest_main)
add_test(NAME BayesianNetworkTests COMMAND bayesian_network_tests)
