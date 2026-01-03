add_library(tracker SHARED
    kalman_filter.cpp
    extended_kalman_filter.cpp
    unscented_kalman_filter.cpp
    sequential_monte_carlo.cpp
)
target_link_libraries(tracker PRIVATE Eigen3::Eigen)
target_include_directories(tracker PUBLIC ${CMAKE_SOURCE_DIR}/include)