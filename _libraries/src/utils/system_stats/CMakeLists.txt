add_library(system_stats
    ../system_stats.cpp
)

target_include_directories(system_stats PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
)
