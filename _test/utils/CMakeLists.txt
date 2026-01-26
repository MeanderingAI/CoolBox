# Utilities/Other Tests

set(JSON_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/packages/IO/dataformats/json/libjson.dylib")
set(GTEST_MAIN_PATH "${CMAKE_SOURCE_DIR}/../build/lib/libgtest_main.a")
set(GTEST_PATH "${CMAKE_SOURCE_DIR}/../build/lib/libgtest.a")
add_executable(json_tests test_json.cpp)
target_link_libraries(json_tests PRIVATE ${JSON_LIB_PATH} ${GTEST_MAIN_PATH} ${GTEST_PATH} pthread)
target_include_directories(json_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME JsonTests COMMAND json_tests)


set(DISTRIBUTED_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/IO/distributed/libdistributed.a")
add_executable(distributed_fs_tests test_distributed_fs.cpp)
target_link_libraries(distributed_fs_tests PRIVATE ${DISTRIBUTED_LIB_PATH} ${GTEST_MAIN_PATH} ${GTEST_PATH} pthread)
target_include_directories(distributed_fs_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DistributedFSTests COMMAND distributed_fs_tests)


set(GTEST_INCLUDE_PATH "/Users/mehranghamaty/wkspace/ExplorerTools/build/googletest-src/googletest/include")

set(FUZZER_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/security/fuzzer/libfuzzer.a")
add_executable(fuzzer_tests test_fuzzer.cpp)
target_link_libraries(fuzzer_tests PRIVATE ${FUZZER_LIB_PATH} ${GTEST_MAIN_PATH} ${GTEST_PATH} pthread)
target_include_directories(fuzzer_tests PRIVATE ${CMAKE_SOURCE_DIR}/include ${GTEST_INCLUDE_PATH})
add_test(NAME FuzzerTests COMMAND fuzzer_tests)
