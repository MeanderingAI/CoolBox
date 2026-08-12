add_library(bayesian_quantum_control SHARED
    source/quantum_control_bo.cpp
)
target_include_directories(bayesian_quantum_control PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_include_directories(bayesian_quantum_control PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../headers)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(bayesian_quantum_control_tests
        tests/test_quantum_control_bo.cpp
    )
    target_include_directories(bayesian_quantum_control_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../headers)
    target_link_libraries(bayesian_quantum_control_tests PRIVATE bayesian_quantum_control tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(bayesian_quantum_control_tests)
    add_test(NAME BayesianQuantumControlTests COMMAND bayesian_quantum_control_tests)
endif()