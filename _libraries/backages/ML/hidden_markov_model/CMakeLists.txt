cmake_minimum_required(VERSION 3.12)
project(hidden_markov_model LANGUAGES CXX)

find_package(Eigen3 REQUIRED)
find_package(GTest QUIET)

add_library(hidden_markov_model SHARED
    source/hidden_markov_model.cpp
)
target_include_directories(hidden_markov_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(hidden_markov_model PRIVATE Eigen3::Eigen)

## Tests
enable_testing()

# Only add tests if testing is enabled and GTest is available as an imported target.
if(BUILD_TESTING AND TARGET GTest::gtest_main)
    add_executable(hidden_markov_model_tests
        tests/test_hidden_markov_model.cpp
    )
    target_link_libraries(hidden_markov_model_tests PRIVATE hidden_markov_model GTest::gtest_main)
    add_test(NAME HiddenMarkovModelTests COMMAND hidden_markov_model_tests)
else()
    message(STATUS "GTest::gtest_main not available or BUILD_TESTING is OFF; hidden_markov_model tests will be skipped")
endif()
