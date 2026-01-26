add_library(deep_learning SHARED
    source/layer.cpp
    source/loss.cpp
    source/neural_network.cpp
    source/optimizer.cpp
    source/templates.cpp
    source/tensor.cpp
)
target_include_directories(deep_learning PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
