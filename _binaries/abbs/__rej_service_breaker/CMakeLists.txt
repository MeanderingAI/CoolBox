add_executable(service_breaker
    service_breaker.cpp
)

target_include_directories(service_breaker PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(service_breaker PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "service_breaker_bin"
)
