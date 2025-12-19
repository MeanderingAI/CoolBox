add_library(deep_learning SHARED
    tensor.cpp
    layer.cpp
    loss.cpp
    optimizer.cpp
    neural_network.cpp
    templates.cpp
)

target_include_directories(deep_learning PUBLIC ${CMAKE_SOURCE_DIR}/include)
