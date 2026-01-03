# Add service_breaker as a library target for linking
add_library(service_breaker STATIC service_breaker/service_breaker.cpp)
target_include_directories(service_breaker PUBLIC
    ${CMAKE_SOURCE_DIR}/_libraries/include
    ${CMAKE_SOURCE_DIR}/_binaries/apps/include
    ${CMAKE_SOURCE_DIR}/_binaries/apps/service_breaker
)
set_target_properties(service_breaker PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
# Import system_monitor library from services
add_library(system_monitor_lib STATIC IMPORTED)
set_target_properties(system_monitor_lib PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/_binaries/services/system_monitor/libsystem_monitor_lib.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/_binaries/services/system_monitor/include"
)
# Add url_shortener as a library target for linking
add_library(url_shortener STATIC url_shortener/url_shortener.cpp)
target_include_directories(url_shortener PUBLIC
    ${CMAKE_SOURCE_DIR}/_libraries/include
    ${CMAKE_SOURCE_DIR}/_binaries/apps/include
    ${CMAKE_SOURCE_DIR}/_binaries/apps/url_shortener
)
set_target_properties(url_shortener PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
add_subdirectory(service_manager)
add_executable(matlab_platform_app matlab_platform_app/matlab_platform_app.cpp)
target_include_directories(matlab_platform_app PRIVATE
    ${CMAKE_SOURCE_DIR}/_libraries/include
    ${CMAKE_SOURCE_DIR}/_binaries/apps/include
    ${CMAKE_SOURCE_DIR}/_binaries/services/system_monitor/include
    ${CMAKE_SOURCE_DIR}/_binaries/apps/generated
)
target_link_libraries(matlab_platform_app PRIVATE
    auth html_processor url_shortener system_monitor_lib service_breaker pthread
)
set_target_properties(matlab_platform_app PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
cmake_minimum_required(VERSION 3.10)
project(service_manager)

add_subdirectory(server_launch)
set(CMAKE_CXX_STANDARD 17)


include_directories(
    ${CMAKE_SOURCE_DIR}/_libraries/include
    ${CMAKE_SOURCE_DIR}/_binaries/apps/service_manager/include
)



