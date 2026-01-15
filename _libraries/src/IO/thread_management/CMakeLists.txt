add_library(thread_management
    thread_pool.cpp
)

target_include_directories(thread_management PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
)
