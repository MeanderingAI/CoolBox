add_library(genetic_search SHARED
    source/genetic_search.cpp
)
target_include_directories(genetic_search PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_include_directories(genetic_search PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../headers)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(genetic_search_tests
        tests/test_genetic_search.cpp
    )
    target_link_libraries(genetic_search_tests PRIVATE genetic_search tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(genetic_search_tests)
    add_test(NAME GeneticSearchTests COMMAND genetic_search_tests)
endif()
