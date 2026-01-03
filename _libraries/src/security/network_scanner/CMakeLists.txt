cmake_minimum_required(VERSION 3.15)

add_library(network_scanner STATIC 
    network_scanner.cpp
)

target_include_directories(network_scanner PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(network_scanner PUBLIC fuzzer)
target_compile_features(network_scanner PUBLIC cxx_std_17)
