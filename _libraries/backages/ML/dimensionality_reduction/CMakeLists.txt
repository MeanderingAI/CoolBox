add_library(dimensionality_reduction SHARED
    source/knn.cpp
    source/pca.cpp
    source/svd.cpp
    source/umap.cpp
)
target_include_directories(dimensionality_reduction PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Tests
enable_testing()
add_executable(dimensionality_reduction_tests
    tests/test_pca.cpp
    tests/test_svd.cpp
    tests/test_umap.cpp
)
target_link_libraries(dimensionality_reduction_tests PRIVATE dimensionality_reduction GTest::gtest_main)
add_test(NAME DimensionalityReductionTests COMMAND dimensionality_reduction_tests)
