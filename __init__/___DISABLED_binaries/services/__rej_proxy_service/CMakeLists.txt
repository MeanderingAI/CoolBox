# CMakeLists.txt for proxy_service

add_library(proxy_service_lib
    src/proxy_service.cpp
)

target_include_directories(proxy_service_lib PUBLIC include)
