add_library(support_vector_machine SHARED
    source/linear_kernel.cpp
    source/polynomial_kernel.cpp
    source/rbf_kernel.cpp
    source/sigmoid_kernel.cpp
    source/support_vector_machine.cpp
)
target_include_directories(support_vector_machine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(support_vector_machine PUBLIC mytrix)

# Tests
if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(support_vector_machine_tests
        tests/test_support_vector_machine.cpp
    )
    target_link_libraries(support_vector_machine_tests PRIVATE support_vector_machine tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(support_vector_machine_tests)
    add_test(NAME SupportVectorMachineTests COMMAND support_vector_machine_tests)
endif()
