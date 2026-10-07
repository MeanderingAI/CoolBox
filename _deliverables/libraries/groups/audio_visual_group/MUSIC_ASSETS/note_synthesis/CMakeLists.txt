cmake_minimum_required(VERSION 3.16)

# note_synthesis — waveform generators and envelope (ADSR) for musical notes.
add_library(note_synthesis SHARED
    source/oscillator.cpp
    source/envelope.cpp
    source/note_generator.cpp
)

target_compile_features(note_synthesis PUBLIC cxx_std_17)
target_include_directories(note_synthesis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(note_synthesis_tests tests/test_note_synthesis.cpp)
    target_link_libraries(note_synthesis_tests PRIVATE note_synthesis tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(note_synthesis_tests)
    add_test(NAME NoteSynthesisTests COMMAND note_synthesis_tests)
endif()
