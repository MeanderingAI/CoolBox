# Servlets Tests
add_executable(servlets_tests test_servlets.cpp)
target_link_libraries(servlets_tests PRIVATE servlets gtest_main pthread)
target_include_directories(servlets_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ServletsTests COMMAND servlets_tests)
