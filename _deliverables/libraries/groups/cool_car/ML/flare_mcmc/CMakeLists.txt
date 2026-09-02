add_library(flare_mcmc SHARED
    source/flare_mcmc.cpp
)
target_include_directories(flare_mcmc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(flare_mcmc PUBLIC cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(flare_mcmc_tests
        tests/test_flare_mcmc.cpp
    )
    target_link_libraries(flare_mcmc_tests PRIVATE flare_mcmc tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(flare_mcmc_tests)
    add_test(NAME FlareMcmcTests COMMAND flare_mcmc_tests)
endif()