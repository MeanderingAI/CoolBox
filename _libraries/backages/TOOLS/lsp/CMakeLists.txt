cmake_minimum_required(VERSION 4.3.1)
project(plang_lsp)


add_library(plang_lsp_lib
	source/lsp_server.cpp
)

target_include_directories(plang_lsp_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers
	${CMAKE_CURRENT_SOURCE_DIR}/../../PARSER/programming_language/headers)

# Link against the plang parser implementation so symbols are available to LSP code
target_link_libraries(plang_lsp_lib PUBLIC plang)

option(BUILD_TESTS "Build tests for plang_lsp" ON)
if(BUILD_TESTS)
	enable_testing()
	add_executable(plang_lsp_test tests/test_lsp.cpp)
	target_link_libraries(plang_lsp_test PRIVATE plang_lsp_lib)
	add_test(NAME plang_lsp_test COMMAND plang_lsp_test)
endif()
