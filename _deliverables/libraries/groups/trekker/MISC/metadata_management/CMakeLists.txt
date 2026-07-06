cmake_minimum_required(VERSION 3.12)
project(metadata_management)
add_library(metadata_management SHARED
    source/metadata_management.cpp
)
target_include_directories(metadata_management PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Tests
if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(metadata_management_tests
        tests/test_metadata_management.cpp
    )
    target_link_libraries(metadata_management_tests PRIVATE metadata_management tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(metadata_management_tests)
    add_test(NAME MetadataManagementTests COMMAND metadata_management_tests)
endif()
