cmake_minimum_required(VERSION 3.16)

add_library(sip_server SHARED
    source/sip_server.cpp
)

target_compile_features(sip_server PUBLIC cxx_std_17)
target_include_directories(sip_server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

target_link_libraries(sip_server PUBLIC
    servlets
    http_server
)
