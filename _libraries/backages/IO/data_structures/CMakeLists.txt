# Data Structures Library

add_library(data_structures SHARED
    source/binary_search_tree.cpp
    source/linked_list.cpp
    source/hash_map.cpp
    source/concurrent_hash_map.cpp
    source/concurrent_linked_list.cpp
)

target_include_directories(data_structures PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    /Users/mehranghamaty/wkspace/ExplorerTools/_binaries/services/service_cache/__include
    /Users/mehranghamaty/wkspace/ExplorerTools/_binaries/services/service_dns/__include
    /Users/mehranghamaty/wkspace/ExplorerTools/_binaries/services/__rej_proxy_service/include
)

target_compile_features(data_structures PUBLIC cxx_std_17)

# Link pthread for concurrent data structures
find_package(Threads REQUIRED)
target_link_libraries(data_structures PRIVATE Threads::Threads)

# Tests
enable_testing()
add_executable(data_structures_tests
    tests/test_binary_search_tree.cpp
    tests/test_linked_list.cpp
    tests/test_hash_map.cpp
    tests/test_concurrent_hash_map.cpp
    tests/test_concurrent_linked_list.cpp
)
target_link_libraries(data_structures_tests PRIVATE data_structures Threads::Threads)
target_include_directories(data_structures_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/headers)
add_test(NAME DataStructuresTests COMMAND data_structures_tests)
