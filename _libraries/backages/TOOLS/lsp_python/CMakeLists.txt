cmake_minimum_required(VERSION 3.5)
project(plpython_lsp)

add_library(plpython_lsp_lib
    source/lsp_server_python.cpp
)

target_include_directories(plpython_lsp_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../../PARSER/python/headers)

option(BUILD_TESTS "Build tests for plpython_lsp" ON)
if(BUILD_TESTS)
    enable_testing()
    add_executable(plpython_lsp_test tests/test_lsp_python.cpp)
    target_link_libraries(plpython_lsp_test PRIVATE plpython_lsp_lib)
    add_test(NAME plpython_lsp_test COMMAND plpython_lsp_test)
endif()
