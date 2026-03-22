cmake_minimum_required(VERSION 3.5)
project(pljava_lsp)

add_library(pljava_lsp_lib
    source/lsp_server_java.cpp
)

target_include_directories(pljava_lsp_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../../PARSER/java/headers)

option(BUILD_TESTS "Build tests for pljava_lsp" ON)
if(BUILD_TESTS)
    enable_testing()
    add_executable(pljava_lsp_test tests/test_lsp_java.cpp)
    target_link_libraries(pljava_lsp_test PRIVATE pljava_lsp_lib)
    add_test(NAME pljava_lsp_test COMMAND pljava_lsp_test)
endif()
