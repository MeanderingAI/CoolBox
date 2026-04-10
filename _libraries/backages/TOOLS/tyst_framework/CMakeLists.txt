cmake_minimum_required(VERSION 3.12)

project(tyst_framework LANGUAGES CXX)

add_library(tyst_framework INTERFACE)
target_include_directories(tyst_framework INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(tyst_framework INTERFACE cxx_std_17)

if(TARGET GTest::gtest)
    target_link_libraries(tyst_framework INTERFACE GTest::gtest)
endif()

if(TARGET GTest::gtest_main)
    add_library(tyst_framework_main INTERFACE)
    target_link_libraries(tyst_framework_main INTERFACE tyst_framework GTest::gtest_main)
endif()

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(tyst_framework_tests
        tests/test_tyst_framework.cpp
    )
    target_link_libraries(tyst_framework_tests PRIVATE tyst_framework_main)
    add_test(NAME TystFrameworkTests COMMAND tyst_framework_tests)
endif()