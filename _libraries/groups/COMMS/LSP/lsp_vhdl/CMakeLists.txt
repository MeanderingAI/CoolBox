cmake_minimum_required(VERSION 4.2.3)
project(plvhdl_lsp)

add_library(plvhdl_lsp_lib
    source/lsp_server_vhdl.cpp
)

target_include_directories(plvhdl_lsp_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../../PARSER/vhdl/include)
# If the vhdl parser package lives next to this TOOLS package, add it so
# the `vhdl_parser` target exists and can be linked.
target_link_libraries(plvhdl_lsp_lib PUBLIC vhdl_parser)

option(BUILD_TESTS_PLVHDL "Build tests for plvhdl_lsp" ON)
if(BUILD_TESTS_PLVHDL)
    enable_testing()
    add_executable(plvhdl_lsp_test tests/test_lsp_vhdl.cpp)
    target_link_libraries(plvhdl_lsp_test PRIVATE plvhdl_lsp_lib)
    target_include_directories(plvhdl_lsp_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/headers ${CMAKE_CURRENT_SOURCE_DIR}/../../PARSER/vhdl/include)
    add_test(NAME plvhdl_lsp_test COMMAND plvhdl_lsp_test)
endif()
