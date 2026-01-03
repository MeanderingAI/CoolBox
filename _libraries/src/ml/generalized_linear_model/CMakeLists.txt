add_library(generalized_linear_model SHARED
    generalized_linear_model.cpp
    linear_regression.cpp
)
target_link_libraries(generalized_linear_model PRIVATE Eigen3::Eigen)
target_include_directories(generalized_linear_model PUBLIC ${CMAKE_SOURCE_DIR}/include)