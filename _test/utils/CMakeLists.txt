# Utilities/Other Tests
add_executable(json_tests test_json.cpp)
target_link_libraries(json_tests PRIVATE json gtest_main pthread)
target_include_directories(json_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME JsonTests COMMAND json_tests)

add_executable(distributed_fs_tests test_distributed_fs.cpp)
target_link_libraries(distributed_fs_tests PRIVATE distributed_fs gtest_main pthread)
target_include_directories(distributed_fs_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DistributedFSTests COMMAND distributed_fs_tests)

add_executable(fuzzer_tests test_fuzzer.cpp)
target_link_libraries(fuzzer_tests PRIVATE fuzzer pthread)
target_include_directories(fuzzer_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME FuzzerTests COMMAND fuzzer_tests)
