add_library(simulated_annealing SHARED
    source/simulated_annealing.cpp
)
target_include_directories(simulated_annealing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_include_directories(simulated_annealing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../headers)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(simulated_annealing_tests
        tests/test_simulated_annealing.cpp
    )
    target_link_libraries(simulated_annealing_tests PRIVATE simulated_annealing tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(simulated_annealing_tests)
    add_test(NAME SimulatedAnnealingTests COMMAND simulated_annealing_tests)
endif()
