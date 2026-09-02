cmake_minimum_required(VERSION 3.20)
project(xlsx_writer)

add_library(xlsx_writer
    source/zip_writer.cpp
    source/xlsx_writer.cpp
)

target_include_directories(xlsx_writer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

target_compile_features(xlsx_writer PUBLIC cxx_std_17)

option(BUILD_TESTS_XLSX_WRITER "Build xlsx_writer tests" ON)
if(BUILD_TESTS_XLSX_WRITER)
    enable_testing()
    add_executable(xlsx_writer_test tests/test_xlsx_writer.cpp)
    target_link_libraries(xlsx_writer_test PRIVATE xlsx_writer)
    add_test(NAME xlsx_writer_test COMMAND xlsx_writer_test)
endif()

install(TARGETS xlsx_writer)
