add_library(tracker SHARED
    source/extended_kalman_filter.cpp
    source/kalman_filter.cpp
    source/sequential_monte_carlo.cpp
    source/unscented_kalman_filter.cpp
)
target_include_directories(tracker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
