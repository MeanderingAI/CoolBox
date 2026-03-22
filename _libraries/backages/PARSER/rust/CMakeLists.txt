cmake_minimum_required(VERSION 3.5)
project(prust)

add_library(plrust_parser_lib
    source/lexer_rust.cpp
    source/parser_rust.cpp
)

target_include_directories(plrust_parser_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

option(BUILD_TESTS "Build tests for prust" ON)
if(BUILD_TESTS)
    enable_testing()
    add_executable(prust_test tests/test_rust.cpp)
    target_link_libraries(prust_test PRIVATE plrust_parser_lib)
    add_test(NAME prust_test COMMAND prust_test)
endif()
