cmake_minimum_required(VERSION 3.16)

add_library(quantum_simulator SHARED
    source/gates.cpp
    source/quantum_state.cpp
    source/circuit.cpp
    source/qft.cpp
    source/algorithms.cpp
    source/shor.cpp
)

target_compile_features(quantum_simulator PUBLIC cxx_std_17)
target_include_directories(quantum_simulator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_definitions(quantum_simulator PRIVATE $<$<CXX_COMPILER_ID:MSVC>:_USE_MATH_DEFINES>)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()

    add_executable(quantum_simulator_tests tests/test_quantum_simulator.cpp)
    target_link_libraries(quantum_simulator_tests PRIVATE quantum_simulator tyst_framework_main)
    target_compile_definitions(quantum_simulator_tests PRIVATE $<$<CXX_COMPILER_ID:MSVC>:_USE_MATH_DEFINES>)
    add_test(NAME QuantumSimulatorTests COMMAND quantum_simulator_tests)

    add_executable(quantum_algorithms_tests tests/test_quantum_algorithms.cpp)
    target_link_libraries(quantum_algorithms_tests PRIVATE quantum_simulator tyst_framework_main)
    add_test(NAME QuantumAlgorithmsTests COMMAND quantum_algorithms_tests)
endif()
