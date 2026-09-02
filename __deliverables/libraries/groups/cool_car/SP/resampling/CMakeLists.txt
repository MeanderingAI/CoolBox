cmake_minimum_required(VERSION 3.12)

add_library(resampling SHARED
    source/resampling.cpp
)

target_include_directories(resampling PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(resampling PUBLIC cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(resampling_tests
        tests/test_resampling.cpp
    )
    target_link_libraries(resampling_tests PRIVATE resampling tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(resampling_tests)
    add_test(NAME ResamplingTests COMMAND resampling_tests)
endif()
