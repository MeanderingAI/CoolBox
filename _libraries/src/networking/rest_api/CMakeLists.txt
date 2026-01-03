add_library(rest_api SHARED
    server.cpp
    model_server.cpp
    http_protocol.cpp
    http_server_base.cpp
    http1_servlet.cpp
    http2_server.cpp
    http3_server.cpp
    unified_http_server.cpp
)

target_include_directories(rest_api PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${QUICHE_INCLUDE_DIR}
)

add_dependencies(rest_api quiche_build)
target_link_libraries(rest_api
    json
    pthread
    networking_http_request_response
    advanced_logging
    ${QUICHE_LIB}
    file_system_file_watcher
)
