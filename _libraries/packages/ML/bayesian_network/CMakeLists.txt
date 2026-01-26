add_library(bayesian_network SHARED
    source/bayesian_network.cpp
)
target_include_directories(bayesian_network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
