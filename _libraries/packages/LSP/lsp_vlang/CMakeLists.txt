cmake_minimum_required(VERSION 4.3.1)
project(plvlang_lsp)

add_library(plvlang_lsp_lib
    source/lsp_server_vlang.cpp
)

target_include_directories(plvlang_lsp_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

option(BUILD_TESTS_PLVLANG "Build tests for plvlang_lsp" ON)
if(BUILD_TESTS_PLVLANG)
    enable_testing()
    add_executable(plvlang_lsp_test tests/test_lsp_vlang.cpp)
    target_link_libraries(plvlang_lsp_test PRIVATE plvlang_lsp_lib)
    target_include_directories(plvlang_lsp_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/headers
    )
    add_test(NAME plvlang_lsp_test COMMAND plvlang_lsp_test)
endif()