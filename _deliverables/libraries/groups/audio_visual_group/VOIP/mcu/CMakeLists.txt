cmake_minimum_required(VERSION 3.16)

add_library(voip_mcu SHARED
    source/mcu.cpp
)

target_compile_features(voip_mcu PUBLIC cxx_std_17)
target_include_directories(voip_mcu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
