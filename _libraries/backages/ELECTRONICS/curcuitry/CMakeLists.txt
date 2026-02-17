cmake_minimum_required(VERSION 3.14)
project(curcuitry LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------
# Library
# ------------------------------------------------------------------
add_library(curcuitry
    src/circuit.cpp
)

target_include_directories(curcuitry
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Eigen (header-only)
if(EXISTS "${CMAKE_SOURCE_DIR}/eigen-src")
    target_include_directories(curcuitry PUBLIC "${CMAKE_SOURCE_DIR}/eigen-src")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/../eigen-src")
    target_include_directories(curcuitry PUBLIC "${CMAKE_SOURCE_DIR}/../eigen-src")
else()
    find_package(Eigen3 REQUIRED)
    target_link_libraries(curcuitry PUBLIC Eigen3::Eigen)
endif()

# ------------------------------------------------------------------
# Tests (Google Test)
# ------------------------------------------------------------------
add_executable(curcuitry_tests
    tests/circuit_test.cpp
)
target_link_libraries(curcuitry_tests PRIVATE curcuitry GTest::gtest_main)
add_test(NAME CurcuitryTests COMMAND curcuitry_tests)
