# knn is header-only
add_library(knn INTERFACE)
target_include_directories(knn INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Link Eigen (fetched at top level)
if(TARGET Eigen3::Eigen)
    target_link_libraries(knn INTERFACE Eigen3::Eigen)
elseif(TARGET eigen)
    target_link_libraries(knn INTERFACE eigen)
endif()

# Tests
if(BUILD_TESTING)
    add_executable(knn_tests tests/test_knn.cpp)
    target_link_libraries(knn_tests PRIVATE knn GTest::gtest_main)
    add_test(NAME KNNTests COMMAND knn_tests)
endif()
