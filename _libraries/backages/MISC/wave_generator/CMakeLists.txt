cmake_minimum_required(VERSION 3.12)

add_library(wave_generator_utils SHARED
    source/wave_generator.cpp
)

target_include_directories(wave_generator_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(wave_generator_utils PUBLIC cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(wave_generator_utils_tests
        tests/test_wave_generator.cpp
    )
    target_link_libraries(wave_generator_utils_tests PRIVATE wave_generator_utils tyst_framework_main)
    add_test(NAME WaveGeneratorUtilsTests COMMAND wave_generator_utils_tests)
endif()
