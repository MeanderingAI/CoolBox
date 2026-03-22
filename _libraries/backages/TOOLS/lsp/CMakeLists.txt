cmake_minimum_required(VERSION 3.5)
project(plang_lsp)


add_library(plang_lsp_lib
	source/lsp_server.cpp
)

target_include_directories(plang_lsp_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers
	${CMAKE_CURRENT_SOURCE_DIR}/../programming_language/headers)

option(BUILD_TESTS "Build tests for plang_lsp" ON)
if(BUILD_TESTS)
	enable_testing()
	add_executable(plang_lsp_test tests/test_lsp.cpp)
	target_link_libraries(plang_lsp_test PRIVATE plang_lsp_lib)
	add_test(NAME plang_lsp_test COMMAND plang_lsp_test)
endif()
