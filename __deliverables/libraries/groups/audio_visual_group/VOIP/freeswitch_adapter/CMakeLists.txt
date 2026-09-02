cmake_minimum_required(VERSION 3.16)

add_library(voip_freeswitch_adapter SHARED
    source/freeswitch_adapter.cpp
)

target_compile_features(voip_freeswitch_adapter PUBLIC cxx_std_17)
target_include_directories(voip_freeswitch_adapter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

if(WIN32)
    target_link_libraries(voip_freeswitch_adapter PUBLIC ws2_32)
endif()
