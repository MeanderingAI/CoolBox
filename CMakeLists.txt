# Option to control Emscripten MODULARIZE (default ON, can override with -DEMSCRIPTEN_MODULARIZE=OFF)
option(EMSCRIPTEN_MODULARIZE "Build Emscripten modules with MODULARIZE=1 (async JS loading)" ON)

cmake_minimum_required(VERSION 3.12)
project(Models)


# Ensure std::filesystem is available on macOS
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment version")
endif()

# Add _libraries/include and quiche include as global include directories
include_directories(${CMAKE_SOURCE_DIR}/_libraries/include)
include_directories(${CMAKE_SOURCE_DIR}/external/quiche/quiche/include)

# Set C++ standard globally
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include the external dependencies from the sub-directory
include(cmake/ExternalDependencies.cmake)

# The main executable for the example application
add_executable(tui main.cpp)

# Add subdirectories for the shared library and tests

# Pass top-level source dir to gen_docs
set(TOOLBOX_SOURCE_DIR "${CMAKE_SOURCE_DIR}" CACHE PATH "Top-level ToolBox source directory" FORCE)

add_subdirectory(_libraries/src)
add_subdirectory(test)
add_subdirectory(_binaries/demos)
add_subdirectory(_libraries/gen_docs)

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

add_subdirectory(_binaries/examples)
add_subdirectory(_binaries/apps)
add_subdirectory(_binaries/services)
add_subdirectory(_libraries/src/advanced_logging)


# Add quiche (QUIC/HTTP3) as external dependency
include(ExternalProject)
ExternalProject_Add(
    quiche_build
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/quiche
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cargo build --release --features ffi
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS ${CMAKE_SOURCE_DIR}/external/quiche/target/release/libquiche.a
    BUILD_IN_SOURCE 1
)

# Add quiche include and library paths for dependent targets
set(QUICHE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/quiche/quiche/include)
set(QUICHE_LIB ${CMAKE_SOURCE_DIR}/external/quiche/target/release/libquiche.a)
