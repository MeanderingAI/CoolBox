cmake_minimum_required(VERSION 3.16)

add_library(video_codec SHARED
    source/pixel_format.cpp
    source/yuv_rgb.cpp
    source/frame.cpp
    source/plane.cpp
)

target_compile_features(video_codec PUBLIC cxx_std_17)
target_include_directories(video_codec PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(video_codec_tests tests/test_video_codec.cpp)
    target_link_libraries(video_codec_tests PRIVATE video_codec tyst_framework_main)
    add_test(NAME VideoCodecTests COMMAND video_codec_tests)
endif()
