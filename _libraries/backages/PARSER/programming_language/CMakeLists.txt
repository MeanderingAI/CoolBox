cmake_minimum_required(VERSION 4.3.1)
project(programming_language)


add_library(plang
    source/lexer.cpp
    source/parser.cpp
)

target_include_directories(plang PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

option(BUILD_TESTS "Build tests for programming_language" ON)
if(BUILD_TESTS)
    enable_testing()
    add_executable(plang_test tests/test_plang.cpp)
    target_link_libraries(plang_test plang)
    add_test(NAME plang_test COMMAND plang_test)
endif()

install(TARGETS plang)
