add_library(gabor_patches SHARED
    source/gabor_patches.cpp
)

target_include_directories(gabor_patches PRIVATE
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/cool_car/MATRIX/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)
target_compile_features(gabor_patches PUBLIC cxx_std_17)
target_link_libraries(gabor_patches PUBLIC mytrix)

# Tests
if(BUILD_TESTING)
    enable_testing()
    add_executable(gabor_patches_tests
        tests/test_gabor_patches.cpp
        ${CMAKE_SOURCE_DIR}/_tyst_main/tyst_framework_main.cpp
    )
    target_include_directories(gabor_patches_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers
    )
    target_link_libraries(gabor_patches_tests PRIVATE gabor_patches)
    coolbox_stage_test_runtime_dependencies(gabor_patches_tests)
    add_test(NAME GaborPatchesTests COMMAND gabor_patches_tests)
endif()
