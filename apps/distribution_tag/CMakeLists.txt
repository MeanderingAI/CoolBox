cmake_minimum_required(VERSION 3.20)
project(distribution_tag LANGUAGES CXX)

find_package(Threads REQUIRED)

set(SHARED_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/shared)

# ── Master node ────────────────────────────────────────────────────────────────
add_executable(distribution_tag_master master/main.cpp)
target_compile_features(distribution_tag_master PRIVATE cxx_std_17)
target_include_directories(distribution_tag_master PRIVATE ${SHARED_INCLUDE})
target_link_libraries(distribution_tag_master PRIVATE distributed_compute http_server Threads::Threads)

# ── Worker node ────────────────────────────────────────────────────────────────
add_executable(distribution_tag_worker worker/main.cpp)
target_compile_features(distribution_tag_worker PRIVATE cxx_std_17)
target_include_directories(distribution_tag_worker PRIVATE ${SHARED_INCLUDE})
target_link_libraries(distribution_tag_worker PRIVATE distributed_compute Threads::Threads)

install(TARGETS distribution_tag_master distribution_tag_worker)
