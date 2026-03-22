cmake_minimum_required(VERSION 3.5)
project(plang_js)

add_library(plang_js
    source/lexer_js.cpp
    source/parser_js.cpp
)

target_include_directories(plang_js PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

option(BUILD_TESTS_JS "Build JavaScript parser tests" ON)
if(BUILD_TESTS_JS)
    enable_testing()
    add_executable(plang_js_test tests/test_js.cpp)
    target_link_libraries(plang_js_test PRIVATE plang_js)
    add_test(NAME plang_js_test COMMAND plang_js_test)
endif()

install(TARGETS plang_js)
