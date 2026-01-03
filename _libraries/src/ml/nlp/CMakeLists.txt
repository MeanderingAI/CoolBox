cmake_minimum_required(VERSION 3.14)

# NLP library
add_library(nlp
    text_processor.cpp
    embeddings.cpp
)

target_include_directories(nlp PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(nlp PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
