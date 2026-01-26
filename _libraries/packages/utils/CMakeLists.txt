# CMakeLists.txt for utils library
add_library(utils_thread_pool
    thread_pool/thread_pool.h
)
target_include_directories(utils_thread_pool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool)
