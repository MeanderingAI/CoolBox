add_library(hidden_markov_model SHARED
    source/hidden_markov_model.cpp
)
target_include_directories(hidden_markov_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
