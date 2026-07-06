add_library(hill_climbing SHARED
    source/hill_climbing.cpp
)
target_include_directories(hill_climbing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_include_directories(hill_climbing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../headers)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(hill_climbing_tests
        tests/test_hill_climbing.cpp
    )
    target_link_libraries(hill_climbing_tests PRIVATE hill_climbing tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(hill_climbing_tests)
    add_test(NAME HillClimbingTests COMMAND hill_climbing_tests)
endif()
