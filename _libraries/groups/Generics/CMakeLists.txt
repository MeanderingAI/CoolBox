cmake_minimum_required(VERSION 4.2.3)

project(GENERICS VERSION 1.0 LANGUAGES C CXX)

if(NOT TARGET tyst_framework_main)
	add_subdirectory(tyst_framework ${CMAKE_BINARY_DIR}/generics_tyst_framework_build)
endif()