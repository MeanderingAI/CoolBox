add_library(time_series SHARED
    source/time_series.cpp
)
target_include_directories(time_series PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
