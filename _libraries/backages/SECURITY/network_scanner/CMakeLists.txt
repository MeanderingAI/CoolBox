add_library(network_scanner SHARED
    source/network_scanner.cpp
)
target_include_directories(network_scanner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(network_scanner PUBLIC fuzzer)

if(WIN32)
    target_link_libraries(network_scanner PUBLIC ws2_32)
endif()
