cmake_minimum_required(VERSION 3.16)

add_library(audio_container SHARED
    source/ima_adpcm.cpp
    source/wav_io.cpp
)

target_compile_features(audio_container PUBLIC cxx_std_17)
target_include_directories(audio_container PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(audio_container PUBLIC audio_processing)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(audio_container_tests tests/test_audio_container.cpp)
    target_link_libraries(audio_container_tests PRIVATE audio_container audio_processing tyst_framework_main)
    add_test(NAME AudioContainerTests COMMAND audio_container_tests)
endif()
