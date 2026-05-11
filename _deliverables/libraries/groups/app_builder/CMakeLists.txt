# CMakeLists.txt for the app_builder
cmake_minimum_required(VERSION 3.20)

project(APP_BUILDER VERSION 1.0 LANGUAGES C CXX)


# Reference the GRAPHICS and OS_GENERICS sub-packages
add_subdirectory(GRAPHICS)
add_subdirectory(OS_GENERICS)
