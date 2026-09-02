cmake_minimum_required(VERSION 3.20)
project(pdf_reader)

add_library(pdf_reader
    source/inflate.cpp
    source/pdf_reader.cpp
)

target_include_directories(pdf_reader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

target_compile_features(pdf_reader PUBLIC cxx_std_17)

option(BUILD_TESTS_PDF_READER "Build pdf_reader tests" ON)
if(BUILD_TESTS_PDF_READER)
    enable_testing()
    add_executable(pdf_reader_test tests/test_pdf_reader.cpp)
    target_link_libraries(pdf_reader_test PRIVATE pdf_reader)
    add_test(NAME pdf_reader_test COMMAND pdf_reader_test)
endif()

install(TARGETS pdf_reader)
