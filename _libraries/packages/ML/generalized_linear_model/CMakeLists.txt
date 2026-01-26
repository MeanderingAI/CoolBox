add_library(generalized_linear_model SHARED
    source/generalized_linear_model.cpp
    source/linear_regression.cpp
)
target_include_directories(generalized_linear_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
