add_library(hidden_markov_model SHARED
    source/hidden_markov_model.cpp
)
target_include_directories(hidden_markov_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Tests
enable_testing()
add_executable(hidden_markov_model_tests
    tests/test_hidden_markov_model.cpp
)
target_link_libraries(hidden_markov_model_tests PRIVATE hidden_markov_model GTest::gtest_main)
add_test(NAME HiddenMarkovModelTests COMMAND hidden_markov_model_tests)
