add_library(support_vector_machine SHARED
    source/linear_kernel.cpp
    source/polynomial_kernel.cpp
    source/rbf_kernel.cpp
    source/sigmoid_kernel.cpp
    source/support_vector_machine.cpp
)
target_include_directories(support_vector_machine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Tests
enable_testing()
add_executable(support_vector_machine_tests
    tests/test_support_vector_machine.cpp
)
target_link_libraries(support_vector_machine_tests PRIVATE support_vector_machine GTest::gtest_main)
add_test(NAME SupportVectorMachineTests COMMAND support_vector_machine_tests)
