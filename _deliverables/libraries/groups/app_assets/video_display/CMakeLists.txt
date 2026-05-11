cmake_minimum_required(VERSION 3.12)

project(video_display LANGUAGES CXX)

add_library(video_display_lib
    source/frame_buffer.cpp
    source/overlay.cpp
    source/video_player.cpp
)

target_include_directories(video_display_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(video_display_lib PUBLIC cxx_std_17)
set_target_properties(video_display_lib PROPERTIES EXPORT_NAME video_display)

# Link against audio_visual_group libraries when available.
foreach(_dep video_codec video_filters)
    if(TARGET ${_dep})
        target_link_libraries(video_display_lib PUBLIC ${_dep})
    endif()
endforeach()

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(video_display_tests
        tests/test_video_display.cpp
    )
    target_link_libraries(video_display_tests PRIVATE video_display_lib tyst_framework_main)
    add_test(NAME VideoDisplayTests COMMAND video_display_tests)
endif()
