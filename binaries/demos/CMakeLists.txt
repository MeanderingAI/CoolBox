# Demos
add_executable(data_structures_demo data_structures_demo.cpp)
target_link_libraries(data_structures_demo PRIVATE data_structures)
target_include_directories(data_structures_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(concurrent_demo concurrent_demo.cpp)
target_link_libraries(concurrent_demo PRIVATE data_structures pthread)
target_include_directories(concurrent_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(cache_demo cache_demo.cpp)
target_link_libraries(cache_demo PRIVATE cache_server data_structures pthread)
target_include_directories(cache_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(ml_demo ml_demo.cpp)
target_link_libraries(ml_demo PRIVATE probability_distribution)
target_include_directories(ml_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(server_launch_demo server_launch_demo.cpp)
target_link_libraries(server_launch_demo PRIVATE 
    server_launch 
    cache_server 
    dns_server 
    proxy_server 
    data_structures 
    pthread
)
target_include_directories(server_launch_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(html_demo html_demo.cpp)
target_link_libraries(html_demo PRIVATE html_processor)
target_include_directories(html_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(website_demo website_demo.cpp)
target_link_libraries(website_demo PRIVATE html_processor)
target_include_directories(website_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(webserver_demo webserver_demo.cpp)
target_link_libraries(webserver_demo PRIVATE html_processor)
target_include_directories(webserver_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(dfs_demo dfs_demo.cpp)
target_link_libraries(dfs_demo PRIVATE distributed_fs pthread)
target_include_directories(dfs_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(mail_server_demo mail_server_demo.cpp)
target_link_libraries(mail_server_demo PRIVATE mail_server pthread)
target_include_directories(mail_server_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(webcomponents_demo webcomponents_demo.cpp)
target_link_libraries(webcomponents_demo PRIVATE html_processor)
target_include_directories(webcomponents_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(component_webserver_demo component_webserver_demo.cpp)
target_link_libraries(component_webserver_demo PRIVATE html_processor pthread)
target_include_directories(component_webserver_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(mail_webui_demo mail_webui_demo.cpp)
target_link_libraries(mail_webui_demo PRIVATE mail_server html_processor pthread)
target_include_directories(mail_webui_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(ml_webui_demo ml_webui_demo.cpp)
target_link_libraries(ml_webui_demo PRIVATE ml_server html_processor pthread)
target_include_directories(ml_webui_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(security_scanner_demo security_scanner_demo.cpp)
target_link_libraries(security_scanner_demo PRIVATE security pthread)
target_include_directories(security_scanner_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Generate HTML resources
set(HTML_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/html")
set(HTML_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${HTML_GENERATED_DIR})

# Convert HTML files to C++ headers
add_custom_command(
    OUTPUT ${HTML_GENERATED_DIR}/app_launcher_html.h
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/resources/html_to_cpp.py 
            ${HTML_RESOURCES_DIR}/app_launcher.html 
            ${HTML_GENERATED_DIR}/app_launcher_html.h
            APP_LAUNCHER_HTML
    DEPENDS ${HTML_RESOURCES_DIR}/app_launcher.html
    COMMENT "Generating app_launcher_html.h"
)

add_executable(matlab_platform_demo 
    matlab_platform_demo.cpp
    ${HTML_GENERATED_DIR}/app_launcher_html.h
)
target_link_libraries(matlab_platform_demo PRIVATE auth html_processor url_shortener system_monitor service_breaker pthread)
target_include_directories(matlab_platform_demo PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${HTML_GENERATED_DIR}
)

# Service Manager - Standalone shared library management tool (disabled for now)
# add_executable(service_manager 
#     ../apps/service_manager.cpp
# )
# target_link_libraries(service_manager PRIVATE pthread)
# target_include_directories(service_manager PRIVATE 
#     ${CMAKE_SOURCE_DIR}/include
#     ${HTML_GENERATED_DIR}
# )

# Binary Manager - Standalone executable/binary management tool
add_executable(binary_manager 
    binary_manager.cpp
)
target_link_libraries(binary_manager PRIVATE pthread)
target_include_directories(binary_manager PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${HTML_GENERATED_DIR}
)


add_executable(scanner_webui_demo scanner_webui_demo.cpp)
target_link_libraries(scanner_webui_demo PRIVATE host_scanner html_processor pthread)
target_include_directories(scanner_webui_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(fuzzer_demo fuzzer_demo.cpp)
target_link_libraries(fuzzer_demo PRIVATE fuzzer pthread)
target_include_directories(fuzzer_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(network_fuzzer_demo network_fuzzer_demo.cpp)
target_link_libraries(network_fuzzer_demo PRIVATE fuzzer pthread)
target_include_directories(network_fuzzer_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(network_scanner_demo network_scanner_demo.cpp)
target_link_libraries(network_scanner_demo PRIVATE network_scanner fuzzer pthread)
target_include_directories(network_scanner_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Set C++ standard for all demos
set_target_properties(
    data_structures_demo
    concurrent_demo
    cache_demo
    ml_demo
    server_launch_demo
    html_demo
    website_demo
    webserver_demo
    dfs_demo
    mail_server_demo
    webcomponents_demo
    scanner_webui_demo
    matlab_platform_demo
    component_webserver_demo
    mail_webui_demo
    ml_webui_demo
    security_scanner_demo
    fuzzer_demo
    network_fuzzer_demo
    network_scanner_demo
    PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
