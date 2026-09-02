cmake_minimum_required(VERSION 3.12)

add_library(filters SHARED
    source/filters.cpp
)

target_include_directories(filters PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(filters PUBLIC cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(filters_tests
        tests/test_filters.cpp
    )
    target_link_libraries(filters_tests PRIVATE filters tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(filters_tests)
    add_test(NAME FiltersTests COMMAND filters_tests)
endif()
