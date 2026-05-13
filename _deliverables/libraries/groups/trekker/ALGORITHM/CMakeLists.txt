cmake_minimum_required(VERSION 3.20)

# ──────────────────────────────────────────────────────────────────────────
# trekker::ALGORITHM
# ──────────────────────────────────────────────────────────────────────────
# Header-only library — no compiled sources; consumers simply add the
# include directories and link against algorithm_headers.
# ──────────────────────────────────────────────────────────────────────────

add_library(algorithm_headers INTERFACE)

target_include_directories(algorithm_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/lists/sorts
    ${CMAKE_CURRENT_SOURCE_DIR}/lists/woodford
    ${CMAKE_CURRENT_SOURCE_DIR}/graphs/a_star
    ${CMAKE_CURRENT_SOURCE_DIR}/graphs/bfs
    ${CMAKE_CURRENT_SOURCE_DIR}/graphs/dfs
    ${CMAKE_CURRENT_SOURCE_DIR}/graphs/dijkstra
)

target_compile_features(algorithm_headers INTERFACE cxx_std_17)

# ──────────────────────────────────────────────────────────────────────────
# Tests
# ──────────────────────────────────────────────────────────────────────────

if(BUILD_TESTING)
    add_executable(sorts_tests
        lists/sorts/test_sorts.cpp
    )

    target_link_libraries(sorts_tests
        PRIVATE
            algorithm_headers
            tyst_framework_main
    )

    target_include_directories(sorts_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lists/sorts
    )

    add_test(NAME SortsTests COMMAND sorts_tests)
endif()
