add_library(network_scanner SHARED
    source/network_scanner.cpp
)
target_include_directories(network_scanner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
