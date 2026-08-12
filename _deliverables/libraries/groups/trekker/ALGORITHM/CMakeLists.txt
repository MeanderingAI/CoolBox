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
    ${CMAKE_CURRENT_SOURCE_DIR}/scheduling
    ${CMAKE_CURRENT_SOURCE_DIR}/graphs/a_star
    ${CMAKE_CURRENT_SOURCE_DIR}/graphs/bfs
    ${CMAKE_CURRENT_SOURCE_DIR}/graphs/dfs
    ${CMAKE_CURRENT_SOURCE_DIR}/graphs/dijkstra
    ${CMAKE_CURRENT_SOURCE_DIR}/graphs/tsp
    ${CMAKE_CURRENT_SOURCE_DIR}/dynamic_programming/rank_convergence_ltdp
)

target_compile_features(algorithm_headers INTERFACE cxx_std_17)

# ──────────────────────────────────────────────────────────────────────────
# Tests
# ──────────────────────────────────────────────────────────────────────────

if(BUILD_TESTING)
    add_executable(sorts_tests
        lists/sorts/test_sorts.cpp
        ${CMAKE_SOURCE_DIR}/_tyst_main/tyst_framework_main.cpp
    )

    target_link_libraries(sorts_tests
        PRIVATE
            algorithm_headers
    )

    target_include_directories(sorts_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lists/sorts
        ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers
    )

    add_test(NAME SortsTests COMMAND sorts_tests)

    add_executable(scheduling_tests
        scheduling/test_scheduling.cpp
        ${CMAKE_SOURCE_DIR}/_tyst_main/tyst_framework_main.cpp
    )

    target_link_libraries(scheduling_tests
        PRIVATE
            algorithm_headers
    )

    target_include_directories(scheduling_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/scheduling
        ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers
    )

    add_test(NAME SchedulingTests COMMAND scheduling_tests)
endif()
