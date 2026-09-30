cmake_minimum_required(VERSION 3.16)

add_library(video_container SHARED
    source/gif_lzw.cpp
    source/gif_quantize.cpp
    source/gif_encoder.cpp
    source/avi_writer.cpp
)

target_compile_features(video_container PUBLIC cxx_std_17)
target_include_directories(video_container PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(video_container PUBLIC video_codec)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(video_container_tests tests/test_video_container.cpp)
    target_link_libraries(video_container_tests PRIVATE video_container video_codec tyst_framework_main)
    add_test(NAME VideoContainerTests COMMAND video_container_tests)
endif()
