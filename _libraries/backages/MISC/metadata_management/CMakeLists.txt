add_library(metadata_management SHARED
    source/metadata_management.cpp
)
target_include_directories(metadata_management PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Tests
enable_testing()
add_executable(metadata_management_tests
    tests/test_metadata_management.cpp
)
target_link_libraries(metadata_management_tests PRIVATE metadata_management GTest::gtest_main)
add_test(NAME MetadataManagementTests COMMAND metadata_management_tests)
