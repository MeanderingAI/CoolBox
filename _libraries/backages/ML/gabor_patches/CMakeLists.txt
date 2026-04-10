add_library(gabor_patches SHARED
    source/gabor_patches.cpp
)

target_include_directories(gabor_patches PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)
target_compile_features(gabor_patches PUBLIC cxx_std_17)
target_link_libraries(gabor_patches PUBLIC mytrix)

# Tests
if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(gabor_patches_tests tests/test_gabor_patches.cpp)
    target_link_libraries(gabor_patches_tests PRIVATE gabor_patches tyst_framework_main)
    add_test(NAME GaborPatchesTests COMMAND gabor_patches_tests)
endif()
