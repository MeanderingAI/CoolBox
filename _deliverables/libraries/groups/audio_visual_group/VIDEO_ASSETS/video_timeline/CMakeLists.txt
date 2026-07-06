cmake_minimum_required(VERSION 3.16)

add_library(video_timeline SHARED
    source/clip.cpp
    source/track.cpp
    source/timeline.cpp
)

target_compile_features(video_timeline PUBLIC cxx_std_17)
target_include_directories(video_timeline PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(video_timeline_tests tests/test_video_timeline.cpp)
    target_link_libraries(video_timeline_tests PRIVATE video_timeline tyst_framework_main)
    add_test(NAME VideoTimelineTests COMMAND video_timeline_tests)
endif()
