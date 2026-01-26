
# Only add executable, not a library, as there are no standalone sources for a library
add_executable(cache_server_service
	__src/cache_server.cpp
	__src/distributed_cache.cpp
	/Users/mehranghamaty/wkspace/ToolTool/_libraries/packages/IO/data_structures/source/concurrent_hash_map.cpp
	/Users/mehranghamaty/wkspace/ToolTool/_libraries/packages/IO/data_structures/source/concurrent_linked_list.cpp
)
target_include_directories(cache_server_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/__include)
target_include_directories(cache_server_service PRIVATE /Users/mehranghamaty/wkspace/ToolTool/_libraries/packages/IO/data_structures/headers)
target_include_directories(cache_server_service PRIVATE /Users/mehranghamaty/wkspace/ToolTool/_binaries/services/service_cache/__include)
target_include_directories(cache_server_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../service_cache/__include)

