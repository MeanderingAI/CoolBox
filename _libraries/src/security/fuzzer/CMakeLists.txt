cmake_minimum_required(VERSION 3.15)

add_library(fuzzer STATIC 
    fuzzer.cpp
)

target_include_directories(fuzzer PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_features(fuzzer PUBLIC cxx_std_17)
