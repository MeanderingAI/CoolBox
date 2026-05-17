
# Data Structures Library
set(DATA_STRUCTURES_SOURCES
	trees/source/binary_search_tree.cpp
	trees/source/red_black_tree.cpp
	trees/source/splay_tree.cpp
	trees/source/heap.cpp
	trees/source/log_structured_merge_tree.cpp
	trees/source/maple_tree.cpp
	trees/source/btree.cpp
	list/source/linked_list.cpp
	maps/source/hash_map.cpp
	maps/source/concurrent_hash_map.cpp
	list/source/concurrent_linked_list.cpp
	filters/source/bloom_filter.cpp
	filters/source/egh_filter.cpp
	trees/source/van_emde_boas.cpp
)

add_library(data_structures ${DATA_STRUCTURES_SOURCES})

target_include_directories(data_structures PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/headers
	${CMAKE_CURRENT_SOURCE_DIR}/trees/headers
	${CMAKE_CURRENT_SOURCE_DIR}/maps/headers
	${CMAKE_CURRENT_SOURCE_DIR}/list/headers
	${CMAKE_CURRENT_SOURCE_DIR}/filters/headers
)

target_compile_features(data_structures PUBLIC cxx_std_17)

find_package(Threads REQUIRED)
target_link_libraries(data_structures PRIVATE Threads::Threads)

# Optional: link thread_pool_utils if available
if(TARGET thread_pool_utils)
	target_link_libraries(data_structures PRIVATE thread_pool_utils)
endif()

if(BUILD_TESTING)
	add_executable(data_structures_tests
		trees/tests/test_binary_search_tree.cpp
		trees/tests/test_btree.cpp
		trees/tests/test_splay_tree.cpp
		trees/tests/test_heap.cpp
		trees/tests/test_log_structured_merge_tree.cpp
		trees/tests/maple_tree_test.cpp
		trees/tests/test_radix_tree.cpp
		list/tests/test_linked_list.cpp
		list/tests/test_folio_queue.cpp
		maps/tests/test_hash_map.cpp
		maps/tests/test_concurrent_hash_map.cpp
		list/tests/test_concurrent_linked_list.cpp
		filters/tests/test_bloom_filter.cpp
		filters/tests/test_egh_filter.cpp	trees/tests/test_van_emde_boas.cpp
	list/tests/test_priority_queue.cpp	)
	target_link_libraries(data_structures_tests PRIVATE data_structures tyst_framework_main Threads::Threads)
	if(TARGET thread_pool_utils)
		target_link_libraries(data_structures_tests PRIVATE thread_pool_utils)
	endif()
	target_include_directories(data_structures_tests PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/headers
		${CMAKE_CURRENT_SOURCE_DIR}/trees/headers
		${CMAKE_CURRENT_SOURCE_DIR}/maps/headers
		${CMAKE_CURRENT_SOURCE_DIR}/list/headers
		${CMAKE_CURRENT_SOURCE_DIR}/filters/headers
	)
	add_test(NAME DataStructuresTests COMMAND data_structures_tests)
endif()

# Matrix library
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/matrix" AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/matrix")
	add_subdirectory(matrix)
endif()
add_subdirectory(xarray)
