add_library(deep_learning SHARED
    source/layer.cpp
    source/loss.cpp
    source/neural_network.cpp
    source/optimizer.cpp
    source/templates.cpp
    source/tensor.cpp
)
target_include_directories(deep_learning PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Tests
enable_testing()
add_executable(deep_learning_tests
    tests/test_deep_learning.cpp
)
target_link_libraries(deep_learning_tests PRIVATE deep_learning GTest::gtest_main)
add_test(NAME DeepLearningTests COMMAND deep_learning_tests)
