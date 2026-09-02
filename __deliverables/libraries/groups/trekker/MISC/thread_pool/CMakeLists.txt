cmake_minimum_required(VERSION 3.12)
# Build a reusable thread pool library
add_library(thread_pool_utils STATIC
    source/thread_pool.cpp
)
target_include_directories(thread_pool_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Tests
enable_testing()
add_executable(thread_pool_tests
    tests/test_thread_pool.cpp
    ${CMAKE_SOURCE_DIR}/_tyst_main/tyst_framework_main.cpp
)
target_link_libraries(thread_pool_tests PRIVATE thread_pool_utils)
target_include_directories(thread_pool_tests SYSTEM PRIVATE
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers
)
add_test(NAME ThreadPoolTests COMMAND thread_pool_tests)

set_target_properties(thread_pool_utils PROPERTIES PUBLIC_HEADER "headers/thread_pool.h")
