cmake_minimum_required(VERSION 3.16)

add_library(audio_sync SHARED
    source/av_timestamp.cpp
    source/sync_controller.cpp
    source/lip_sync.cpp
)

target_compile_features(audio_sync PUBLIC cxx_std_17)
target_include_directories(audio_sync PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(audio_sync PRIVATE audio_processing)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(audio_sync_tests tests/test_audio_sync.cpp)
    target_link_libraries(audio_sync_tests PRIVATE audio_sync audio_processing tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(audio_sync_tests)
    add_test(NAME AudioSyncTests COMMAND audio_sync_tests)
endif()
