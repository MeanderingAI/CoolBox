add_library(networking_http_request_response
    request_response.cpp
)

target_include_directories(networking_http_request_response PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/networking/http
)
