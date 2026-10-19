cmake_minimum_required(VERSION 3.20)

project(generics_timer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------
# Library (header-only)
# ------------------------------------------------------------------
add_library(generics_timer INTERFACE)

target_include_directories(generics_timer INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)
target_compile_features(generics_timer INTERFACE cxx_std_17)

# ------------------------------------------------------------------
# Tests (tyst framework)
# ------------------------------------------------------------------
find_package(Threads REQUIRED)

add_executable(generics_timer_tests
    tests/test_timer.cpp
    ${CMAKE_SOURCE_DIR}/_tyst_main/tyst_framework_main.cpp
)
target_include_directories(generics_timer_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers
)
target_link_libraries(generics_timer_tests PRIVATE generics_timer Threads::Threads)
add_test(NAME GenericsTimerTests COMMAND generics_timer_tests)
