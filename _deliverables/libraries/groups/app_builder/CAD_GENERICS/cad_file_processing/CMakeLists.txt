cmake_minimum_required(VERSION 3.12)

project(cad_file_processing LANGUAGES CXX)

add_library(cad_file_processing STATIC
    source/cad_file_processing.cpp
)

target_include_directories(cad_file_processing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(cad_file_processing PUBLIC cxx_std_17)

add_executable(cad_file_processing_demo
    tools/cad_file_processing_demo.cpp
)

target_link_libraries(cad_file_processing_demo PRIVATE cad_file_processing)
target_compile_features(cad_file_processing_demo PRIVATE cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(cad_file_processing_tests
        tests/test_cad_file_processing.cpp
    )
    target_link_libraries(cad_file_processing_tests PRIVATE cad_file_processing tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(cad_file_processing_tests)
    add_test(NAME CadFileProcessingTests COMMAND cad_file_processing_tests)
endif()
