# Data Structures Tests

set(DATA_STRUCTURES_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/IO/data_structures/libdata_structures.dylib")
set(GTEST_MAIN_PATH "${CMAKE_SOURCE_DIR}/../build/lib/libgtest_main.a")
set(GTEST_PATH "${CMAKE_SOURCE_DIR}/../build/lib/libgtest.a")
add_executable(data_structures_tests
    test_binary_search_tree.cpp
    test_linked_list.cpp
    test_hash_map.cpp
    test_concurrent_hash_map.cpp
    test_concurrent_linked_list.cpp)
target_link_libraries(data_structures_tests PRIVATE ${DATA_STRUCTURES_LIB_PATH} ${GTEST_MAIN_PATH} ${GTEST_PATH} pthread)
target_include_directories(data_structures_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DataStructuresTests COMMAND data_structures_tests)
