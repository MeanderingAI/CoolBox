# CMakeLists.txt for system_monitor service

add_library(system_monitor_lib
    src/system_monitor.cpp
)

target_include_directories(system_monitor_lib PUBLIC include)
