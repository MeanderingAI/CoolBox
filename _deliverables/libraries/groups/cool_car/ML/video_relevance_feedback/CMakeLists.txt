add_library(video_relevance_feedback SHARED
    source/video_relevance_feedback.cpp
)
target_include_directories(video_relevance_feedback PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(video_relevance_feedback PUBLIC cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(video_relevance_feedback_tests
        tests/test_video_relevance_feedback.cpp
    )
    target_link_libraries(video_relevance_feedback_tests PRIVATE video_relevance_feedback tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(video_relevance_feedback_tests)
    add_test(NAME VideoRelevanceFeedbackTests COMMAND video_relevance_feedback_tests)
endif()