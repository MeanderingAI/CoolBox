cmake_minimum_required(VERSION 3.12)
project(hidden_markov_model LANGUAGES CXX)

add_library(hidden_markov_model SHARED
    source/hidden_markov_model.cpp
)
target_include_directories(hidden_markov_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(hidden_markov_model PUBLIC mytrix)

## Tests
enable_testing()

# Only add tests if testing is enabled and the internal test wrapper is available.
if(BUILD_TESTING AND TARGET tyst_framework_main)
    add_executable(hidden_markov_model_tests
        tests/test_hidden_markov_model.cpp
    )
    target_link_libraries(hidden_markov_model_tests PRIVATE hidden_markov_model tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(hidden_markov_model_tests)
    add_test(NAME HiddenMarkovModelTests COMMAND hidden_markov_model_tests)
else()
    message(STATUS "tyst_framework_main not available or BUILD_TESTING is OFF; hidden_markov_model tests will be skipped")
endif()
