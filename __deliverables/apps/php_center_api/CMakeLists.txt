cmake_minimum_required(VERSION 3.20)
project(php_center_api LANGUAGES CXX)

find_package(Threads REQUIRED)

set(PHP_CENTER_API_INCLUDES
    ${CMAKE_SOURCE_DIR}/__deliverables/libraries/groups/trekker/IO/http_server/headers
    ${CMAKE_SOURCE_DIR}/__deliverables/libraries/groups/trekker/IO/servlets/headers
)

add_executable(php_center_api main.cpp)
target_compile_features(php_center_api PRIVATE cxx_std_17)
target_include_directories(php_center_api PRIVATE ${PHP_CENTER_API_INCLUDES})
target_link_libraries(php_center_api PRIVATE
    plang_php
    http_server
    servlets
    Threads::Threads
)

install(TARGETS php_center_api)
