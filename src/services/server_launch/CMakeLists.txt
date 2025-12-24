# Server Launch library
add_library(server_launch SHARED
    service_manager.cpp
)

target_include_directories(server_launch PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(server_launch
    pthread
)

set_target_properties(server_launch PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
