cmake_minimum_required(VERSION 3.20)

project(generics_memory LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Overriding global operator new/delete affects every allocation in the
# final binary, so it is opt-in. When OFF, MemoryStats is still usable but
# nothing feeds it automatically.
option(GENERICS_MEMORY_ENABLE_STATS
    "Override global operator new/delete to track process-wide allocation statistics" OFF)

add_library(generics_memory STATIC
    source/memory_stats.cpp
)

target_include_directories(generics_memory PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)
target_compile_features(generics_memory PUBLIC cxx_std_17)

if(GENERICS_MEMORY_ENABLE_STATS)
    target_compile_definitions(generics_memory PUBLIC GENERICS_MEMORY_ENABLE_STATS=1)
endif()

# ------------------------------------------------------------------
# Tests (tyst framework)
# ------------------------------------------------------------------
add_executable(generics_memory_tests
    tests/test_memory.cpp
    ${CMAKE_SOURCE_DIR}/_tyst_main/tyst_framework_main.cpp
)
target_include_directories(generics_memory_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers
)
target_link_libraries(generics_memory_tests PRIVATE generics_memory)
add_test(NAME GenericsMemoryTests COMMAND generics_memory_tests)
