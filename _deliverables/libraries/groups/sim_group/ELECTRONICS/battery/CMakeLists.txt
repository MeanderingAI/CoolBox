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

# Link chemistry interface so consumers (tests) get its include dirs
target_link_libraries(battery INTERFACE chemistry)

# ------------------------------------------------------------------
# Tests (Google Test)
# ------------------------------------------------------------------
add_executable(battery_tests
    tests/battery_test.cpp
    ${CMAKE_SOURCE_DIR}/_tyst_main/tyst_framework_main.cpp
)
target_include_directories(battery_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers
)
target_link_libraries(battery_tests PRIVATE battery)
add_test(NAME BatteryTests COMMAND battery_tests)
