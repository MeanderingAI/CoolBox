add_library(bayesian_network SHARED
    bayesian_network.cpp
)
target_link_libraries(bayesian_network PRIVATE Eigen3::Eigen)
target_include_directories(bayesian_network PUBLIC ${CMAKE_SOURCE_DIR}/include)