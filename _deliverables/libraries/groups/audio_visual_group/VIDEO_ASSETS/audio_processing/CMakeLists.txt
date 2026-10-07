cmake_minimum_required(VERSION 3.16)

add_library(audio_processing SHARED
    source/audio_buffer.cpp
    source/resampler.cpp
    source/mixer.cpp
    source/effects.cpp
)

target_compile_features(audio_processing PUBLIC cxx_std_17)
target_include_directories(audio_processing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

find_package(Threads REQUIRED)
target_link_libraries(audio_processing PRIVATE Threads::Threads)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(audio_processing_tests tests/test_audio_processing.cpp)
    target_link_libraries(audio_processing_tests PRIVATE audio_processing tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(audio_processing_tests)
    add_test(NAME AudioProcessingTests COMMAND audio_processing_tests)
endif()
