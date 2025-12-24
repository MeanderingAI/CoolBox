add_library(hidden_markov_model SHARED 
    hidden_markov_model.cpp
)
target_link_libraries(hidden_markov_model PRIVATE Eigen3::Eigen)
target_include_directories(hidden_markov_model PUBLIC ${CMAKE_SOURCE_DIR}/include)