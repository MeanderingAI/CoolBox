cmake_minimum_required(VERSION 3.12)

project(tyst_framework LANGUAGES CXX)

add_library(tyst_framework INTERFACE)
target_include_directories(tyst_framework INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(tyst_framework INTERFACE cxx_std_17)

add_library(tyst_framework_main STATIC
    source/tyst_framework_main.cpp
)
target_link_libraries(tyst_framework_main PUBLIC tyst_framework)
target_compile_features(tyst_framework_main PUBLIC cxx_std_17)

if(BUILD_TESTING)
    enable_testing()
    add_executable(tyst_framework_tests
        tests/test_tyst_framework.cpp
    )
    target_link_libraries(tyst_framework_tests PRIVATE tyst_framework_main)
    add_test(NAME TystFrameworkTests COMMAND tyst_framework_tests)
endif()