cmake_minimum_required(VERSION 3.16)

add_library(music_sequencer SHARED
    source/sequencer.cpp
    source/pattern.cpp
    source/mixer_bus.cpp
)

target_compile_features(music_sequencer PUBLIC cxx_std_17)
target_include_directories(music_sequencer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(music_sequencer PRIVATE note_synthesis music_theory)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(music_sequencer_tests tests/test_music_sequencer.cpp)
    target_link_libraries(music_sequencer_tests PRIVATE music_sequencer note_synthesis music_theory tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(music_sequencer_tests)
    add_test(NAME MusicSequencerTests COMMAND music_sequencer_tests)
endif()
