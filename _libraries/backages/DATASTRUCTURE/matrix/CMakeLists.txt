add_library(mytrix SHARED
    source/matrix_dense.cpp
    source/matrix_sparse.cpp
    source/matrix_backend.cpp
)

target_include_directories(mytrix PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(mytrix PUBLIC cxx_std_17)

if(NOT TARGET Eigen3::Eigen AND NOT TARGET eigen)
    find_package(Eigen3 QUIET)
endif()

if(TARGET Eigen3::Eigen)
    target_link_libraries(mytrix PUBLIC Eigen3::Eigen)
elseif(TARGET eigen)
    target_link_libraries(mytrix PUBLIC eigen)
endif()

# Optionally link OpenCL/CUDA if available (stubbed for now)
find_package(Threads REQUIRED)
target_link_libraries(mytrix PRIVATE Threads::Threads)

# Tests (stub)
if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(matrix_tests tests/test_matrix.cpp)
    target_link_libraries(matrix_tests PRIVATE mytrix tyst_framework_main)
    add_test(NAME MatrixTests COMMAND matrix_tests)
endif()
