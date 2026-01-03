# System Scanner Library
add_library(host_scanner SHARED
    host_scanner.cpp
)

target_include_directories(host_scanner PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(host_scanner
    pthread
)

set_target_properties(host_scanner PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
