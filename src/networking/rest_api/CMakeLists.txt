add_library(rest_api SHARED
    server.cpp
    model_server.cpp
    http_protocol.cpp
    http_server_base.cpp
    http1_server.cpp
    http2_server.cpp
    http3_server.cpp
)

target_include_directories(rest_api PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(rest_api
    json
    pthread
)
