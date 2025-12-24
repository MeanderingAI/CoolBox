# DNS Server library
add_library(dns_server SHARED
    dns_server.cpp
)

target_include_directories(dns_server PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dns_server
    data_structures
    pthread
)

set_target_properties(dns_server PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
