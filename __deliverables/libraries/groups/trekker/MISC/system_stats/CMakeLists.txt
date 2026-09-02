add_library(system_stats SHARED
    source/system_stats.cpp
)
target_include_directories(system_stats PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
