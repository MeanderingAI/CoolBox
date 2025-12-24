add_library(service_breaker
    service_breaker.cpp
)

target_include_directories(service_breaker PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
