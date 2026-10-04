add_library(vae INTERFACE)
target_include_directories(vae INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(vae INTERFACE mytrix)

# Tests
if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(vae_tests
        tests/test_vae.cpp
    )
    target_link_libraries(vae_tests PRIVATE vae tyst_framework_main)
    add_test(NAME VAETests COMMAND vae_tests)
endif()
