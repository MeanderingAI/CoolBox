add_library(marked_point_process SHARED
    marked_point_process.cpp
    piecewise_conditional_intensity_model.cpp
)

target_link_libraries(marked_point_process PRIVATE Eigen3::Eigen)
target_include_directories(marked_point_process PUBLIC ${CMAKE_SOURCE_DIR}/include/marked_point_process)
