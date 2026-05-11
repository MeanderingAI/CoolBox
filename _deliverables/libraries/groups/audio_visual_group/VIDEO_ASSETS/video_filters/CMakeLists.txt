cmake_minimum_required(VERSION 3.16)

add_library(video_filters SHARED
    source/spatial_filters.cpp
    source/color_grading.cpp
    source/temporal_filters.cpp
)

target_compile_features(video_filters PUBLIC cxx_std_17)
target_include_directories(video_filters PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(video_filters PUBLIC video_codec)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(video_filters_tests tests/test_video_filters.cpp)
    target_link_libraries(video_filters_tests PRIVATE video_filters tyst_framework_main)
    add_test(NAME VideoFiltersTests COMMAND video_filters_tests)
endif()
