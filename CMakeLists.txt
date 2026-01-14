# Option to control Emscripten MODULARIZE (default ON, can override with -DEMSCRIPTEN_MODULARIZE=OFF)
option(EMSCRIPTEN_MODULARIZE "Build Emscripten modules with MODULARIZE=1 (async JS loading)" ON)

cmake_minimum_required(VERSION 3.12)
project(Models)

# Ensure std::filesystem is available on macOS
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment version")
endif()

# Add _libraries/include as global include directory
include_directories(${CMAKE_SOURCE_DIR}/_libraries/include)

# Set C++ standard globally
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include the external dependencies from the sub-directory
include(cmake/ExternalDependencies.cmake)

# Pass top-level source dir to gen_docs
set(TOOLBOX_SOURCE_DIR "${CMAKE_SOURCE_DIR}" CACHE PATH "Top-level ToolBox source directory" FORCE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(_test)
add_subdirectory(_binaries)
add_subdirectory(_libraries)

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





# QUICHE/HTTP3/Unified server support disabled for pure HTTP2 build
