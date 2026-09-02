cmake_minimum_required(VERSION 3.20)
project(plang_php)

add_library(plang_php
    source/lexer_php.cpp
    source/parser_php.cpp
    source/value_php.cpp
    source/interpreter_php.cpp
)

target_include_directories(plang_php PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

target_compile_features(plang_php PUBLIC cxx_std_17)

option(BUILD_TESTS_PHP "Build PHP interpreter tests" ON)
if(BUILD_TESTS_PHP)
    enable_testing()
    add_executable(plang_php_test tests/test_php.cpp)
    target_link_libraries(plang_php_test PRIVATE plang_php)
    add_test(NAME plang_php_test COMMAND plang_php_test)
endif()

install(TARGETS plang_php)
