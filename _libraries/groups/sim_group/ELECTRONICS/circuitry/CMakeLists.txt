cmake_minimum_required(VERSION 3.14)
project(circuitry LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------
# Library
# ------------------------------------------------------------------
add_library(circuitry
    src/circuit.cpp
)

target_include_directories(circuitry
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(circuitry PUBLIC mytrix)

# ------------------------------------------------------------------
# Tests (Google Test)
# ------------------------------------------------------------------
add_executable(circuitry_tests
    tests/circuit_test.cpp
)
target_include_directories(circuitry_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(circuitry_tests PRIVATE circuitry tyst_framework_main)
add_test(NAME CircuitryTests COMMAND circuitry_tests)
