cmake_minimum_required(VERSION 3.20)
project(plscala_lsp)

add_library(plscala_lsp_lib
    source/lsp_server_scala.cpp
)

target_include_directories(plscala_lsp_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

option(BUILD_TESTS_PLSCALA "Build tests for plscala_lsp" ON)
if(BUILD_TESTS_PLSCALA)
    enable_testing()
    add_executable(plscala_lsp_test tests/test_lsp_scala.cpp)
    target_link_libraries(plscala_lsp_test PRIVATE plscala_lsp_lib)
    target_include_directories(plscala_lsp_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/headers
    )
    add_test(NAME plscala_lsp_test COMMAND plscala_lsp_test)
endif()
