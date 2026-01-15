# Servlets Tests

set(SERVLETS_LIB_PATH "${CMAKE_BINARY_DIR}/../_libraries/src/IO/servlets/libservlets.a")
set(GTEST_MAIN_PATH "${CMAKE_SOURCE_DIR}/../build/lib/libgtest_main.a")
set(GTEST_PATH "${CMAKE_SOURCE_DIR}/../build/lib/libgtest.a")
set(GTEST_INCLUDE_PATH "/Users/mehranghamaty/wkspace/ExplorerTools/build/googletest-src/googletest/include")
add_executable(servlets_tests test_servlets.cpp)
target_link_libraries(servlets_tests PRIVATE ${SERVLETS_LIB_PATH} ${GTEST_MAIN_PATH} ${GTEST_PATH} pthread)
target_include_directories(servlets_tests SYSTEM PRIVATE "/Users/mehranghamaty/wkspace/ExplorerTools/_libraries/include")
target_include_directories(servlets_tests PRIVATE ${CMAKE_SOURCE_DIR}/include "/Users/mehranghamaty/wkspace/ExplorerTools/_libraries/include/IO/servlets" ${GTEST_INCLUDE_PATH})
add_test(NAME ServletsTests COMMAND servlets_tests)
