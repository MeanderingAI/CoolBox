add_library(matrix SHARED
    source/matrix_dense.cpp
    source/matrix_sparse.cpp
    source/matrix_backend.cpp
)

target_include_directories(matrix PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(matrix PUBLIC cxx_std_17)

# Optionally link OpenCL/CUDA if available (stubbed for now)
find_package(Threads REQUIRED)
target_link_libraries(matrix PRIVATE Threads::Threads)

# Tests (stub)
if(BUILD_TESTING)
    enable_testing()
    add_executable(matrix_tests tests/test_matrix.cpp)
    target_link_libraries(matrix_tests PRIVATE matrix GTest::gtest_main)
    add_test(NAME MatrixTests COMMAND matrix_tests)
endif()
