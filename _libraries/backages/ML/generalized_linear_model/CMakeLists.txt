add_library(generalized_linear_model SHARED
    source/generalized_linear_model.cpp
    source/linear_regression.cpp
)
target_include_directories(generalized_linear_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Tests
enable_testing()
add_executable(generalized_linear_model_tests
    tests/test_linear_regression.cpp
)
target_link_libraries(generalized_linear_model_tests PRIVATE generalized_linear_model GTest::gtest_main)
add_test(NAME GeneralizedLinearModelTests COMMAND generalized_linear_model_tests)
