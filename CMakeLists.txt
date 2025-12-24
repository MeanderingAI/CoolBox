cmake_minimum_required(VERSION 3.12)
project(Models)

# Set C++ standard globally
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include the external dependencies from the sub-directory
include(cmake/ExternalDependencies.cmake)

# The main executable for the example application
add_executable(tui main.cpp)

# Add subdirectories for the shared library and tests
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(demos)

# Option to build Python bindings
option(BUILD_PYTHON_BINDINGS "Build Python bindings" OFF)

# Add Python bindings if requested
if(BUILD_PYTHON_BINDINGS)
    find_package(pybind11 QUIET)
    if(pybind11_FOUND)
        message(STATUS "Building Python bindings")
        add_subdirectory(python_bindings)
    else()
        message(WARNING "pybind11 not found. Python bindings will not be built.")
        message(STATUS "Install pybind11 with: pip install pybind11[global]")
    endif()
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)