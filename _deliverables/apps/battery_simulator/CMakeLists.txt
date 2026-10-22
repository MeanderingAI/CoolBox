cmake_minimum_required(VERSION 3.13)
project(battery_simulator LANGUAGES CXX)

add_executable(battery_simulator
    cnc/main.cpp
)

target_link_libraries(battery_simulator PRIVATE
    battery
    cli_tools
    optimization_factory
)

target_include_directories(battery_simulator PRIVATE
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/sim_group/ELECTRONICS/battery/include
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/OPT/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/OPT/optimization_factory/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/OPT/genetic_search/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/OPT/hill_climbing/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/OPT/simulated_annealing/headers
)

target_compile_features(battery_simulator PRIVATE cxx_std_17)

# Copy dependent DLLs next to the executable so it runs directly on Windows.
if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
    add_custom_command(TARGET battery_simulator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:battery_simulator> $<TARGET_FILE_DIR:battery_simulator>
        COMMAND_EXPAND_LISTS
    )
endif()
