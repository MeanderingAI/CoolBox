add_library(vae INTERFACE)
target_include_directories(vae INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Link Eigen (fetched at top level)
if(TARGET Eigen3::Eigen)
    target_link_libraries(vae INTERFACE Eigen3::Eigen)
elseif(TARGET eigen)
    target_link_libraries(vae INTERFACE eigen)
endif()

# Tests
enable_testing()
add_executable(vae_tests
    tests/test_vae.cpp
)
target_link_libraries(vae_tests PRIVATE vae GTest::gtest_main)
add_test(NAME VAETests COMMAND vae_tests)
