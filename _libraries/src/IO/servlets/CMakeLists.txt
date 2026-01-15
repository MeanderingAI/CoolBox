add_library(servlets
    http_server.cpp
    HttpVersion.cpp
)

target_include_directories(servlets PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
)
