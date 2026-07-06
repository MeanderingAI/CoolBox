cmake_minimum_required(VERSION 3.12)

add_library(fourier_tranforms SHARED
    source/fourier_tranforms.cpp
)

target_include_directories(fourier_tranforms PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(fourier_tranforms PUBLIC cxx_std_17)
target_link_libraries(fourier_tranforms PUBLIC mytrix)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(fourier_tranforms_tests
        tests/test_fourier_tranforms.cpp
    )
    target_link_libraries(fourier_tranforms_tests PRIVATE fourier_tranforms tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(fourier_tranforms_tests)
    add_test(NAME FourierTranformsTests COMMAND fourier_tranforms_tests)
endif()