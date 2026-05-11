cmake_minimum_required(VERSION 3.15)
project(music_studio LANGUAGES CXX)

set(MUSIC_STUDIO_SOURCES
    src/main.cpp
    src/sequencer_view.cpp
    src/mixer_view.cpp
    src/theory_view.cpp
    src/video_view.cpp
)

add_executable(music_studio ${MUSIC_STUDIO_SOURCES})
target_compile_features(music_studio PRIVATE cxx_std_17)
target_include_directories(music_studio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ── GUI window framework ──────────────────────────────────────────────────────
if(TARGET full_application_window)
    target_link_libraries(music_studio PRIVATE full_application_window)
endif()

# ── app_assets ────────────────────────────────────────────────────────────────
if(TARGET audio_mixer_lib)
    target_link_libraries(music_studio PRIVATE audio_mixer_lib)
    target_compile_definitions(music_studio PRIVATE MUSIC_STUDIO_HAS_AUDIO_MIXER=1)
endif()

if(TARGET video_display_lib)
    target_link_libraries(music_studio PRIVATE video_display_lib)
    target_compile_definitions(music_studio PRIVATE MUSIC_STUDIO_HAS_VIDEO_DISPLAY=1)
endif()

# ── audio_visual_group libraries ──────────────────────────────────────────────
foreach(_dep
    note_synthesis
    music_theory
    music_sequencer
    audio_processing
    audio_sync
    video_codec
    video_filters
    video_timeline
    motion_analysis
)
    if(TARGET ${_dep})
        target_link_libraries(music_studio PRIVATE ${_dep})
    endif()
endforeach()

# ── Runtime DLL copy (Windows) ────────────────────────────────────────────────
if(WIN32)
    foreach(_dep
        full_application_window
        audio_mixer_lib
        video_display_lib
        note_synthesis
        music_theory
        music_sequencer
        audio_processing
        audio_sync
        video_codec
        video_filters
    )
        if(TARGET ${_dep})
            add_custom_command(TARGET music_studio POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:${_dep}>
                    $<TARGET_FILE_DIR:music_studio>
            )
        endif()
    endforeach()
endif()
