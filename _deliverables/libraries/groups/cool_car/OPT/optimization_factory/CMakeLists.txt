add_library(optimization_factory SHARED
    source/optimization_factory.cpp
)
target_include_directories(optimization_factory PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_include_directories(optimization_factory PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../headers)
target_link_libraries(optimization_factory PUBLIC genetic_search simulated_annealing hill_climbing)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(optimization_factory_tests
        tests/test_optimization_factory.cpp
    )
    target_include_directories(optimization_factory_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/../headers
    )
    target_link_libraries(optimization_factory_tests PRIVATE optimization_factory tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(optimization_factory_tests)
    add_test(NAME OptimizationFactoryTests COMMAND optimization_factory_tests)
endif()
