cmake_minimum_required(VERSION 4.2.3)
project(plc3_lsp)

add_library(plc3_lsp_lib
    source/lsp_server_c3.cpp
)

target_include_directories(plc3_lsp_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

option(BUILD_TESTS_PLC3 "Build tests for plc3_lsp" ON)
if(BUILD_TESTS_PLC3)
    enable_testing()
    add_executable(plc3_lsp_test tests/test_lsp_c3.cpp)
    target_link_libraries(plc3_lsp_test PRIVATE plc3_lsp_lib)
    target_include_directories(plc3_lsp_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/headers
    )
    add_test(NAME plc3_lsp_test COMMAND plc3_lsp_test)
endif()