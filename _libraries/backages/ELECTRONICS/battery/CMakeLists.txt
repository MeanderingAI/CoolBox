cmake_minimum_required(VERSION 3.14)
project(battery LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------
# Library (header-only)
# ------------------------------------------------------------------
add_library(battery INTERFACE)

target_include_directories(battery
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ------------------------------------------------------------------
# Tests (Google Test)
# ------------------------------------------------------------------
add_executable(battery_tests
    tests/battery_test.cpp
)
target_link_libraries(battery_tests PRIVATE battery GTest::gtest_main)
add_test(NAME BatteryTests COMMAND battery_tests)
