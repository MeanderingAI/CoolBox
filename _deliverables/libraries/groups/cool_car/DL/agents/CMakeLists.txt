# CMakeLists.txt for DL/agents
#
# LLM-agent and interactive-learning techniques.
# See include/agent_citations.h and
# _internal_documents/engineering_documention/bib/agent_papers.bib.

cmake_minimum_required(VERSION 3.16)

set(DL_AGENTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

add_library(dl_agents STATIC
    src/agent_citations.cpp
    src/bapo.cpp
    src/llf_helix.cpp
    src/reflexion.cpp
    src/steerability.cpp
    src/trace_opto.cpp
)

target_include_directories(dl_agents PUBLIC ${DL_AGENTS_ROOT}/include)
target_compile_features(dl_agents PUBLIC cxx_std_17)

if(BUILD_TESTING)
    enable_testing()
    # The shared tyst target is not always in the configure graph; fall back to
    # the framework main source when it is missing.
    if(TARGET tyst_framework_main)
        add_executable(dl_agents_tests tests/test_agents.cpp)
        target_link_libraries(dl_agents_tests PRIVATE dl_agents tyst_framework_main)
    else()
        add_executable(dl_agents_tests
            tests/test_agents.cpp
            ${CMAKE_SOURCE_DIR}/_tyst_main/tyst_framework_main.cpp
        )
        target_include_directories(dl_agents_tests PRIVATE
            ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers
        )
        target_link_libraries(dl_agents_tests PRIVATE dl_agents)
    endif()

    coolbox_stage_test_runtime_dependencies(dl_agents_tests)
    add_test(NAME DeepLearningAgentTests COMMAND dl_agents_tests)
endif()
