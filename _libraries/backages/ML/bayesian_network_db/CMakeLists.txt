add_library(bayesian_network_db
    source/bayesian_network_db.cpp
)
target_include_directories(bayesian_network_db PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(bayesian_network_db PUBLIC mytrix)

# Tests
if(BUILD_TESTING)
    enable_testing()
    add_executable(bayesian_network_db_tests tests/test_bayesian_network_db.cpp)
    target_link_libraries(bayesian_network_db_tests PRIVATE bayesian_network_db GTest::gtest_main)
    add_test(NAME BayesianNetworkDBTests COMMAND bayesian_network_db_tests)
endif()