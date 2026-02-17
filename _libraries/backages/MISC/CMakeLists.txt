# CMakeLists.txt for utils library
add_library(utils_thread_pool INTERFACE)
target_include_directories(utils_thread_pool INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool)
