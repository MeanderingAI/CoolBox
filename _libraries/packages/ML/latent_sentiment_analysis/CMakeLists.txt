add_library(latent_sentiment_analysis SHARED
    source/latent_sentiment_analysis.cpp
)
target_include_directories(latent_sentiment_analysis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
