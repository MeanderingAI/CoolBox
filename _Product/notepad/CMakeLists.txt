cmake_minimum_required(VERSION 3.15)
project(notepad)

# Example: adjust paths to match your project's graphics library
find_package(PkgConfig QUIET)

# If your graphics library provides a CMake config, prefer that
# find_package(graphics REQUIRED)

add_executable(notepad src/main.cpp)

# Example include and link placeholders — update to actual targets
target_include_directories(notepad PRIVATE ${CMAKE_SOURCE_DIR}/_libraries/backages/GRAPHICS/include)
# target_link_libraries(notepad PRIVATE graphics)
