cmake_minimum_required(VERSION 3.14)

# Time series library
add_library(time_series
    time_series.cpp
)

target_include_directories(time_series PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(time_series PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
