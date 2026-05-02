cmake_minimum_required(VERSION 4.2.3)
project(plmatlab_lsp)

add_library(plmatlab_lsp_lib
    source/lsp_server_matlab.cpp
)

target_include_directories(plmatlab_lsp_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../../PARSER/matlab/include)
# If the matlab parser package lives next to this TOOLS package, add it so
# the `matlab_parser` target exists and can be linked.
target_link_libraries(plmatlab_lsp_lib PUBLIC matlab_parser)

option(BUILD_TESTS_PLMATLAB "Build tests for plmatlab_lsp" ON)
if(BUILD_TESTS_PLMATLAB)
    enable_testing()
    add_executable(plmatlab_lsp_test tests/test_lsp_matlab.cpp)
    target_link_libraries(plmatlab_lsp_test PRIVATE plmatlab_lsp_lib)
    target_include_directories(plmatlab_lsp_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/headers ${CMAKE_CURRENT_SOURCE_DIR}/../../PARSER/matlab/include)
    add_test(NAME plmatlab_lsp_test COMMAND plmatlab_lsp_test)
endif()
