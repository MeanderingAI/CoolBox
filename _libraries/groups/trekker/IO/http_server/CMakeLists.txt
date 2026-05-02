add_library(http_server SHARED
    source/http_server.cpp
    source/HttpVersion.cpp
)

target_compile_features(http_server PUBLIC cxx_std_17)

target_include_directories(http_server PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../dataformats/http/headers
)

target_link_libraries(http_server PUBLIC
    servlets
    advanced_logging
    auth
    thread_pool_utils
)

if(WIN32)
    target_link_libraries(http_server PUBLIC ws2_32)
endif()
