cmake_minimum_required(VERSION 3.12)

add_library(wavelets SHARED
    source/wavelets.cpp
)

target_include_directories(wavelets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(wavelets PUBLIC cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(wavelets_tests
        tests/test_wavelets.cpp
    )
    target_link_libraries(wavelets_tests PRIVATE wavelets tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(wavelets_tests)
    add_test(NAME WaveletsTests COMMAND wavelets_tests)
endif()
