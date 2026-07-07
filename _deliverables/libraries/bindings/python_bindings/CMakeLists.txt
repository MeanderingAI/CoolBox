# Add a CTest entry to run the Python HMM bindings test
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(NAME hmm_python_bindings_test
    COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/test_bindings.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
# Add bindings for new libraries
option(BUILD_PYTHON_HTTP_BINDINGS "Build python http_bindings module" OFF)
if(BUILD_PYTHON_HTTP_BINDINGS)
    add_subdirectory(http)
else()
    message(STATUS "Skipping python http_bindings (BUILD_PYTHON_HTTP_BINDINGS=OFF)")
endif()
add_subdirectory(fuzzer)
cmake_minimum_required(VERSION 3.12)
project(ml_core_python_bindings)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
if(NOT TARGET pybind11::pybind11)
    find_package(pybind11 REQUIRED)
endif()
# Prefer repository-provided Eigen3::Eigen target (from ExternalDependencies).
# Fall back to package discovery only when the target is not already available.
if(NOT TARGET Eigen3::Eigen)
    find_package(Eigen3 QUIET CONFIG)
    if(NOT TARGET Eigen3::Eigen)
        find_package(Eigen3 REQUIRED)
    endif()
endif()
find_package(Python COMPONENTS Interpreter Development REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/bindings/python_bindings/include
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/MATRIX/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor_include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor_include/GRAPHICS/charts/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor_include/MISC/wave_generator/headers
)

# Collect implementation sources that are directly referenced by py_ml_core.
set(ML_BINDING_GLOBS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/generalized_linear_model/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/distributed/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/nlp/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/dimensionality_reduction/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/pde_spde_bindings.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/graphics_misc/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/decision_tree/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/support_vector_machine/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/bayesian_network/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hidden_markov_model/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/multi_arm_bandit/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/computer_vision/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/time_series/*.cpp"
)
file(GLOB_RECURSE ML_BINDING_SOURCES CONFIGURE_DEPENDS ${ML_BINDING_GLOBS})

# Remove any accidental main/test translation units.
list(FILTER ML_BINDING_SOURCES EXCLUDE REGEX "/main\\.cpp$")
list(FILTER ML_BINDING_SOURCES EXCLUDE REGEX "/test_.*\\.cpp$")

# Some sources include headers as <foo.h> where foo.h lives under include/<module>/.
# Keep this list curated to avoid shadowing cool_car DL headers like layer.h/tensor.h.
set(ML_BINDINGS_FLAT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/bayesian_network
    ${CMAKE_CURRENT_SOURCE_DIR}/include/computer_vision
    ${CMAKE_CURRENT_SOURCE_DIR}/include/decision_tree
    ${CMAKE_CURRENT_SOURCE_DIR}/include/dimensionality_reduction
    ${CMAKE_CURRENT_SOURCE_DIR}/include/distributed
    ${CMAKE_CURRENT_SOURCE_DIR}/include/generalized_linear_model
    ${CMAKE_CURRENT_SOURCE_DIR}/include/graphics_misc
    ${CMAKE_CURRENT_SOURCE_DIR}/include/hidden_markov_model
    ${CMAKE_CURRENT_SOURCE_DIR}/include/multi_arm_bandit
    ${CMAKE_CURRENT_SOURCE_DIR}/include/nlp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/support_vector_machine
    ${CMAKE_CURRENT_SOURCE_DIR}/include/time_series
)

# Create the Python module
pybind11_add_module(ml_core 
    py_ml_core.cpp
    ${ML_BINDING_SOURCES}
    ../../groups/cool_car/DL/layers/src/tensor.cpp
    ../../groups/cool_car/DL/wrapper/src/templates.cpp
    ../../groups/cool_car/DL/optimizer/src/optimizer.cpp
    ../../groups/cool_car/DL/wrapper/src/neural_network.cpp
    ../../groups/cool_car/DL/loss/src/loss.cpp
    ../../groups/cool_car/DL/layers/src/layer.cpp
)

# Set properties
target_compile_definitions(ml_core PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})
target_compile_features(ml_core PRIVATE cxx_std_17)

# Link libraries
target_link_libraries(ml_core PRIVATE 
    Eigen3::Eigen
)

target_include_directories(ml_core PRIVATE
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/bindings/python_bindings/include
    ${ML_BINDINGS_FLAT_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/DL/layers/include
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/DL/loss/include
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/DL/optimizer/include
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/DL/wrapper/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor_include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor_include/GRAPHICS/charts/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor_include/MISC/wave_generator/headers
)

# Compiler-specific options
target_compile_definitions(ml_core PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})

if(MSVC)
    target_compile_options(ml_core PRIVATE /W4)
else()
    target_compile_options(ml_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set the output directory
set_target_properties(ml_core PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Add py_circuitry only when the circuitry API is available in headers.
set(CIRCUITRY_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/sim_group/ELECTRONICS/circuitry/include)
set(CIRCUITRY_HEADER ${CIRCUITRY_INCLUDE_DIR}/circuit.h)

set(BUILD_PY_CIRCUITRY ON)
if(EXISTS ${CIRCUITRY_HEADER})
    file(READ ${CIRCUITRY_HEADER} CIRCUITRY_HEADER_CONTENT)
    string(FIND "${CIRCUITRY_HEADER_CONTENT}" "class Circuit" HAS_CIRCUIT_CLASS)
    if(HAS_CIRCUIT_CLASS EQUAL -1)
        set(BUILD_PY_CIRCUITRY OFF)
        message(WARNING "Skipping py_circuitry: circuitry headers do not expose class Circuit yet.")
    endif()
else()
    set(BUILD_PY_CIRCUITRY OFF)
    message(WARNING "Skipping py_circuitry: circuitry header not found.")
endif()

if(BUILD_PY_CIRCUITRY)
    pybind11_add_module(py_circuitry
        electronics/py_circuitry.cpp
    )
    target_include_directories(py_circuitry PRIVATE
        ${CIRCUITRY_INCLUDE_DIR}
    )
    target_link_libraries(py_circuitry PRIVATE Eigen3::Eigen)
    set_target_properties(py_circuitry PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()