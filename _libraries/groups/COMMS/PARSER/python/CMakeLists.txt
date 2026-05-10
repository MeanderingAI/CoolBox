cmake_minimum_required(VERSION 3.20)
project(ppython)

add_library(plpython_parser_lib
    source/lexer_python.cpp
    source/parser_python.cpp
)

target_include_directories(plpython_parser_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/headers)

option(BUILD_TESTS "Build tests for ppython" ON)
if(BUILD_TESTS)
    enable_testing()
    add_executable(ppython_test tests/test_python.cpp)
    target_link_libraries(ppython_test PRIVATE plpython_parser_lib)
    target_include_directories(ppython_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/headers)
    add_test(NAME ppython_test COMMAND ppython_test)
endif()
