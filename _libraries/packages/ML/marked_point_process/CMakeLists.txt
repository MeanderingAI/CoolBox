add_library(marked_point_process SHARED
    source/marked_point_process.cpp
    source/piecewise_conditional_intensity_model.cpp
)
target_include_directories(marked_point_process PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
