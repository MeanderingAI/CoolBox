cmake_minimum_required(VERSION 3.13)
project(battery_simulator LANGUAGES CXX)

add_executable(battery_simulator
    cnc/main.cpp
)

set(sim_group_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/groups/sim_group")
find_package(sim_group REQUIRED)

target_link_libraries(battery_simulator PRIVATE
    CHEMISTRY
    ELECTRONICS
)