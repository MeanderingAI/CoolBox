add_library(knn SHARED
    source/knn.cpp
)
target_include_directories(knn PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Tests
enable_testing()
add_executable(knn_tests
    tests/test_knn.cpp
)
target_link_libraries(knn_tests PRIVATE knn GTest::gtest_main)
add_test(NAME KnnTests COMMAND knn_tests)
