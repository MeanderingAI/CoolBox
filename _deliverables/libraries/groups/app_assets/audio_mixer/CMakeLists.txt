cmake_minimum_required(VERSION 3.12)

project(audio_mixer LANGUAGES CXX)

add_library(audio_mixer_lib
    source/mixer_console.cpp
    source/channel_strip.cpp
    source/vu_meter.cpp
)

target_include_directories(audio_mixer_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(audio_mixer_lib PUBLIC cxx_std_17)
set_target_properties(audio_mixer_lib PROPERTIES EXPORT_NAME audio_mixer)

# Link against audio_visual_group libraries when available.
foreach(_dep audio_processing note_synthesis music_sequencer)
    if(TARGET ${_dep})
        target_link_libraries(audio_mixer_lib PUBLIC ${_dep})
    endif()
endforeach()

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(audio_mixer_tests
        tests/test_audio_mixer.cpp
    )
    target_link_libraries(audio_mixer_tests PRIVATE audio_mixer_lib tyst_framework_main)
    add_test(NAME AudioMixerTests COMMAND audio_mixer_tests)
endif()
