add_library(nlp SHARED
    source/embeddings.cpp
    source/text_processor.cpp
)
target_include_directories(nlp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Tests
enable_testing()

# === Tests ===
