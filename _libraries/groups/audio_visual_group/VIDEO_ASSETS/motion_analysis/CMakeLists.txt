cmake_minimum_required(VERSION 3.16)

add_library(motion_analysis SHARED
    source/optical_flow.cpp
    source/motion_vectors.cpp
    source/scene_detection.cpp
)

target_compile_features(motion_analysis PUBLIC cxx_std_17)
target_include_directories(motion_analysis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(motion_analysis PRIVATE video_codec)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(motion_analysis_tests tests/test_motion_analysis.cpp)
    target_link_libraries(motion_analysis_tests PRIVATE motion_analysis video_codec tyst_framework_main)
    add_test(NAME MotionAnalysisTests COMMAND motion_analysis_tests)
endif()
