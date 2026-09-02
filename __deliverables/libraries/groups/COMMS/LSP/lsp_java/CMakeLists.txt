cmake_minimum_required(VERSION 3.20)
project(pljava_lsp)

add_library(pljava_lsp_lib
    source/lsp_server_java.cpp
)

target_include_directories(pljava_lsp_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../../PARSER/java/headers)

# Link against the java parser implementation to resolve symbols used by the LSP
target_link_libraries(pljava_lsp_lib PUBLIC pljava_parser_lib)

option(BUILD_TESTS "Build tests for pljava_lsp" ON)
if(BUILD_TESTS)
    enable_testing()
    add_executable(pljava_lsp_test tests/test_lsp_java.cpp)
    target_link_libraries(pljava_lsp_test PRIVATE pljava_lsp_lib)
    target_include_directories(pljava_lsp_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/headers ${CMAKE_CURRENT_SOURCE_DIR}/../../PARSER/java/headers)
    add_test(NAME pljava_lsp_test COMMAND pljava_lsp_test)
endif()
