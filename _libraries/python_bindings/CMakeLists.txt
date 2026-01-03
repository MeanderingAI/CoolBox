# Add bindings for new libraries
add_subdirectory(http)
add_subdirectory(fuzzer)
cmake_minimum_required(VERSION 3.12)
project(ml_core_python_bindings)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(pybind11 REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Python COMPONENTS Interpreter Development REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/../include
    ${EIGEN3_INCLUDE_DIR}
)

# Collect all source files from the parent project
file(GLOB_RECURSE ML_SOURCES 
    "${CMAKE_SOURCE_DIR}/../_libraries/src/*.cpp"
)

# Remove any main.cpp files
list(FILTER ML_SOURCES EXCLUDE REGEX "main\\.cpp$")

# Create the Python module
pybind11_add_module(ml_core 
    py_ml_core.cpp
    ${ML_SOURCES}
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