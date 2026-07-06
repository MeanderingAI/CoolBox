cmake_minimum_required(VERSION 3.12)

project(cross_platform_builder LANGUAGES CXX)

add_library(cross_platform_builder STATIC
    source/cross_platform_builder.cpp
)

target_include_directories(cross_platform_builder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(cross_platform_builder PUBLIC cxx_std_17)

add_executable(cross_platform_builder_cli
    tools/cross_platform_builder_cli.cpp
)

target_link_libraries(cross_platform_builder_cli PRIVATE cross_platform_builder cli_tools)
target_compile_features(cross_platform_builder_cli PRIVATE cxx_std_17)

if(BUILD_TESTING)
    enable_testing()
    add_executable(cross_platform_builder_tests
        tests/test_cross_platform_builder.cpp
    )
    target_link_libraries(cross_platform_builder_tests PRIVATE cross_platform_builder)
    add_test(NAME CrossPlatformBuilderTests COMMAND cross_platform_builder_tests)
endif()
