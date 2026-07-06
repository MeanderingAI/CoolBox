# CMakeLists.txt for the app_builder
cmake_minimum_required(VERSION 3.20)

project(APP_BUILDER VERSION 1.0 LANGUAGES C CXX)


# Reference the GRAPHICS, OS_GENERICS, and CAD_GENERICS sub-packages
add_subdirectory(GRAPHICS)
add_subdirectory(OS_GENERICS)
add_subdirectory(CAD_GENERICS)
