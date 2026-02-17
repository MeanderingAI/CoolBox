add_library(http_server SHARED
    source/http_server.cpp
    source/HttpVersion.cpp
)
target_include_directories(http_server PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../dataformats/http/headers
)
