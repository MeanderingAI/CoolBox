cmake_minimum_required(VERSION 3.16)

add_library(dl_agent_runtime STATIC
    src/agent_runtime.cpp
    src/distributed_agent_runtime.cpp
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/bindings/python_bindings/src/distributed/message_passing.cpp
)
target_include_directories(dl_agent_runtime PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/bindings/python_bindings/include
)
target_compile_features(dl_agent_runtime PUBLIC cxx_std_17)
find_package(Threads REQUIRED)
target_link_libraries(dl_agent_runtime PUBLIC Threads::Threads)

if(BUILD_TESTING)
    add_executable(agent_runtime_tests
        test/test_agent_runtime.cpp
    )
    target_include_directories(agent_runtime_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers
    )
    target_link_libraries(agent_runtime_tests PRIVATE dl_agent_runtime)
    add_test(NAME AgentRuntimeTests COMMAND agent_runtime_tests)
endif()
