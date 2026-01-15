
# Only add executable, not a library, as there are no standalone sources for a library
add_executable(proxy_server_service __src/proxy_server.cpp)
target_include_directories(proxy_server_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/__include)
target_include_directories(proxy_server_service PRIVATE ${CMAKE_SOURCE_DIR}/_binaries/services/__rej_proxy_service/include)
target_include_directories(proxy_server_service PRIVATE /Users/mehranghamaty/wkspace/ExplorerTools/_binaries/services/__rej_proxy_service/include)

