cmake_minimum_required(VERSION 3.13)
project(battery_simulator LANGUAGES CXX)

add_executable(battery_simulator
    cnc/main.cpp
)

target_link_libraries(battery_simulator PRIVATE
    battery
    cli_tools
)

target_include_directories(battery_simulator PRIVATE
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/sim_group/ELECTRONICS/battery/include
)

target_compile_features(battery_simulator PRIVATE cxx_std_17)
