cmake_minimum_required(VERSION 3.12)
project(hidden_markov_model LANGUAGES CXX)

add_library(hidden_markov_model SHARED
    source/hmm_flare_adapter.cpp
    source/hidden_markov_model.cpp
)
target_include_directories(hidden_markov_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(hidden_markov_model PUBLIC mytrix flare_mcmc)

## Tests
enable_testing()

# Add tests when BUILD_TESTING is enabled. If the shared tyst target is not
# available in this configure graph, fall back to the framework main source.
if(BUILD_TESTING)
    if(TARGET tyst_framework_main)
        add_executable(hidden_markov_model_tests
            tests/test_hidden_markov_model.cpp
        )
        target_link_libraries(hidden_markov_model_tests PRIVATE hidden_markov_model tyst_framework_main)
    else()
        add_executable(hidden_markov_model_tests
            tests/test_hidden_markov_model.cpp
            ${CMAKE_SOURCE_DIR}/_tyst_main/tyst_framework_main.cpp
        )
        target_include_directories(hidden_markov_model_tests PRIVATE
            ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers
        )
        target_link_libraries(hidden_markov_model_tests PRIVATE hidden_markov_model)
    endif()

    target_include_directories(hidden_markov_model_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/trekker/ALGORITHM/dynamic_programming/rank_convergence_ltdp
    )

    coolbox_stage_test_runtime_dependencies(hidden_markov_model_tests)
    add_test(NAME HiddenMarkovModelTests COMMAND hidden_markov_model_tests)
else()
    message(STATUS "BUILD_TESTING is OFF; hidden_markov_model tests will be skipped")
endif()
