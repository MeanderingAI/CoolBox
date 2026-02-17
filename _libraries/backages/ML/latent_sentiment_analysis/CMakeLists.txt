add_library(latent_sentiment_analysis SHARED
    source/latent_sentiment_analysis.cpp
)
target_include_directories(latent_sentiment_analysis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Tests
enable_testing()
add_executable(latent_sentiment_analysis_tests
    tests/test_latent_sentiment_analysis.cpp
)
target_link_libraries(latent_sentiment_analysis_tests PRIVATE latent_sentiment_analysis GTest::gtest_main)
add_test(NAME LatentSentimentAnalysisTests COMMAND latent_sentiment_analysis_tests)
