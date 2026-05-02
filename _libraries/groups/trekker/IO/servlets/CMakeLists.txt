add_library(servlets SHARED
    source/http1_servlet.cpp
    source/http2_servlet.cpp
    source/http3_servlet.cpp
    source/httpU_servlet.cpp
)
target_include_directories(servlets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
if(TARGET quic_transport)
    target_link_libraries(servlets PUBLIC quic_transport)
endif()
