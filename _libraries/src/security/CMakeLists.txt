# Fuzzer library
add_subdirectory(fuzzer)

# Network Scanner library
add_subdirectory(network_scanner)

# Security Library
add_library(security SHARED
    malware_scanner.cpp
)

target_include_directories(security PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(security
    pthread
)

set_target_properties(security PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
