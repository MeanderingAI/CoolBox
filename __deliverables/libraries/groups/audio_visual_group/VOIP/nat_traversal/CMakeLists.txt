cmake_minimum_required(VERSION 3.16)

add_library(nat_traversal SHARED
    source/nat_traversal.cpp
)

target_compile_features(nat_traversal PUBLIC cxx_std_17)
target_include_directories(nat_traversal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(nat_traversal PUBLIC quic_transport)
