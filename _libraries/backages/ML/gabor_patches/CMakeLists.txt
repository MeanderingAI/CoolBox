# Gabor Patches Library

add_library(gabor_patches SHARED
    source/gabor_patches.cpp
)

target_include_directories(gabor_patches PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

# Link Eigen (fetched at top level)
if(TARGET Eigen3::Eigen)
    target_link_libraries(gabor_patches PUBLIC Eigen3::Eigen)
elseif(TARGET eigen)
    target_link_libraries(gabor_patches PUBLIC eigen)
endif()

# Tests
if(BUILD_TESTING)
    add_executable(gabor_patches_tests tests/test_gabor_patches.cpp)
    target_link_libraries(gabor_patches_tests PRIVATE gabor_patches GTest::gtest_main)
    add_test(NAME GaborPatchesTests COMMAND gabor_patches_tests)
endif()
