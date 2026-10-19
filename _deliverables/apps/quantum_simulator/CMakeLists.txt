cmake_minimum_required(VERSION 3.13)
project(quantum_simulator_app LANGUAGES CXX)

# The CMake *target* is named quantum_simulator_app (the library under
# sim_group/PHYSICS/quantum_simulator already owns the target name
# "quantum_simulator"), but the OUTPUT_NAME below still produces a binary
# called `quantum_simulator`.
add_executable(quantum_simulator_app
    src/main.cpp
    src/demo_runner.cpp
    src/quantum_lab_window.cpp
)
set_target_properties(quantum_simulator_app PROPERTIES OUTPUT_NAME quantum_simulator)

target_link_libraries(quantum_simulator_app PRIVATE
    quantum_simulator
    cli_tools
    full_application_window
    charts
)

target_include_directories(quantum_simulator_app PRIVATE
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/sim_group/PHYSICS/quantum_simulator/headers
)

target_compile_features(quantum_simulator_app PRIVATE cxx_std_17)
