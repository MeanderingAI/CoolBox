# Cache Server library
add_library(cache_server SHARED
    distributed_cache.cpp
    cache_server.cpp
)

target_include_directories(cache_server PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(cache_server
    data_structures
    pthread
)

set_target_properties(cache_server PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
