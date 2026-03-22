cmake_minimum_required(VERSION 3.5)
project(plrust_lsp)

add_library(plrust_lsp_lib
    source/lsp_server_rust.cpp
)

target_include_directories(plrust_lsp_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../../PARSER/rust/headers)

option(BUILD_TESTS "Build tests for plrust_lsp" ON)
if(BUILD_TESTS)
    enable_testing()
    add_executable(plrust_lsp_test tests/test_lsp_rust.cpp)
    target_link_libraries(plrust_lsp_test PRIVATE plrust_lsp_lib)
    add_test(NAME plrust_lsp_test COMMAND plrust_lsp_test)
endif()
