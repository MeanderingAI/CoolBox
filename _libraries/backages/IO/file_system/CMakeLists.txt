add_library(file_system SHARED
    source/file_watcher.cpp
)
target_include_directories(file_system PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
