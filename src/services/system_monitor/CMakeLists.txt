add_library(system_monitor
    system_monitor.cpp
)

target_include_directories(system_monitor PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
