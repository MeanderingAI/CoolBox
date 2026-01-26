

# Only add executable, not a library, as there are no standalone sources for a library
add_executable(dns_server_service __src/dns_server.cpp)
target_include_directories(dns_server_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/__include)
target_include_directories(dns_server_service PRIVATE /Users/mehranghamaty/wkspace/ToolTool/_libraries/packages/IO/data_structures/headers)

