# Distributed Computing Library
# Subpackages: task_queue | consensus | message_bus |
#              distributed_lock | service_registry | circuit_breaker

set(DISTRIBUTED_SOURCES
    task_queue/source/task_queue.cpp
    consensus/source/raft_node.cpp
    message_bus/source/message_bus.cpp
    distributed_lock/source/distributed_lock.cpp
    service_registry/source/service_registry.cpp
    circuit_breaker/source/circuit_breaker.cpp
)

add_library(distributed_compute ${DISTRIBUTED_SOURCES})

target_include_directories(distributed_compute PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/task_queue/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/consensus/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/message_bus/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/distributed_lock/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/service_registry/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/circuit_breaker/headers
)

target_compile_features(distributed_compute PUBLIC cxx_std_17)

find_package(Threads REQUIRED)
target_link_libraries(distributed_compute PRIVATE Threads::Threads)

if(BUILD_TESTING)
    add_executable(distributed_tests
        task_queue/tests/test_task_queue.cpp
        consensus/tests/test_raft_node.cpp
        message_bus/tests/test_message_bus.cpp
        distributed_lock/tests/test_distributed_lock.cpp
        service_registry/tests/test_service_registry.cpp
        circuit_breaker/tests/test_circuit_breaker.cpp
    )
    target_link_libraries(distributed_tests PRIVATE
        distributed_compute tyst_framework_main Threads::Threads)
    target_include_directories(distributed_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/task_queue/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/consensus/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/message_bus/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/distributed_lock/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/service_registry/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/circuit_breaker/headers
    )
    add_test(NAME DistributedTests COMMAND distributed_tests)
endif()
