# Proxy Server library
add_library(proxy_server SHARED
    proxy_server.cpp
)

target_include_directories(proxy_server PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/_binaries/services/proxy_service/include
)

target_link_libraries(proxy_server
    data_structures
    pthread
)

set_target_properties(proxy_server PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
