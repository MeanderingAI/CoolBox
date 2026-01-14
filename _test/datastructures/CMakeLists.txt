# Data Structures Tests
add_executable(data_structures_tests
    test_binary_search_tree.cpp
    test_linked_list.cpp
    test_hash_map.cpp
    test_concurrent_hash_map.cpp
    test_concurrent_linked_list.cpp)
target_link_libraries(data_structures_tests PRIVATE data_structures gtest_main pthread)
target_include_directories(data_structures_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DataStructuresTests COMMAND data_structures_tests)
