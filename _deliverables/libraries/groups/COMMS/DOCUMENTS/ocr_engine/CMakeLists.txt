cmake_minimum_required(VERSION 3.20)
project(ocr_engine)

add_library(ocr_engine
    source/ocr_engine.cpp
)

target_include_directories(ocr_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

target_compile_features(ocr_engine PUBLIC cxx_std_17)

option(BUILD_TESTS_OCR_ENGINE "Build ocr_engine tests" ON)
if(BUILD_TESTS_OCR_ENGINE)
    enable_testing()
    add_executable(ocr_engine_test tests/test_ocr_engine.cpp)
    target_link_libraries(ocr_engine_test PRIVATE ocr_engine)
    add_test(NAME ocr_engine_test COMMAND ocr_engine_test)
endif()

install(TARGETS ocr_engine)
