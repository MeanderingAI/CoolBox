# ML Server Library
add_library(ml_server SHARED
    ml_server.cpp
)

target_include_directories(ml_server PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(ml_server
    pthread
)

set_target_properties(ml_server PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
