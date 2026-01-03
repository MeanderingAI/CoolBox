# Computer Vision Module
add_library(computer_vision
    image.cpp
    transforms.cpp
    pipeline.cpp
    layers.cpp
)

target_include_directories(computer_vision PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Link with deep learning library for CV layers
target_link_libraries(computer_vision
    deep_learning
)

# Set C++ standard
set_property(TARGET computer_vision PROPERTY CXX_STANDARD 17)
