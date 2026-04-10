# knn is header-only
add_library(knn INTERFACE)
target_include_directories(knn INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(knn INTERFACE mytrix)

# Tests
if(BUILD_TESTING AND TARGET tyst_framework_main)
    add_executable(knn_tests tests/test_knn.cpp)
    target_link_libraries(knn_tests PRIVATE knn tyst_framework_main)
    add_test(NAME KNNTests COMMAND knn_tests)
endif()
