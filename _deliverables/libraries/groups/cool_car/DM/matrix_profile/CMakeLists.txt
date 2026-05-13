cmake_minimum_required(VERSION 3.12)

add_library(matrix_profile SHARED
    source/matrix_profile.cpp
)

target_include_directories(matrix_profile PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(matrix_profile PUBLIC cxx_std_17)
target_link_libraries(matrix_profile PUBLIC mytrix)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(matrix_profile_tests
        tests/test_matrix_profile.cpp
    )
    target_link_libraries(matrix_profile_tests PRIVATE matrix_profile tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(matrix_profile_tests)
    add_test(NAME MatrixProfileTests COMMAND matrix_profile_tests)
endif()
