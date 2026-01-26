add_library(computer_vision SHARED
    source/image.cpp
    source/layers.cpp
    source/pipeline.cpp
    source/transforms.cpp
)
target_include_directories(computer_vision PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
