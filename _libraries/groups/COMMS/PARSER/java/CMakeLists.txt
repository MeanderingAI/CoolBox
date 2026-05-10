cmake_minimum_required(VERSION 3.20)
project(pjava)

add_library(pljava_parser_lib
    source/lexer_java.cpp
    source/parser_java.cpp
)

target_include_directories(pljava_parser_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

option(BUILD_TESTS "Build tests for pjava" ON)
if(BUILD_TESTS)
    enable_testing()
    add_executable(pjava_test tests/test_java.cpp)
    target_link_libraries(pjava_test PRIVATE pljava_parser_lib)
    add_test(NAME pjava_test COMMAND pjava_test)
endif()
