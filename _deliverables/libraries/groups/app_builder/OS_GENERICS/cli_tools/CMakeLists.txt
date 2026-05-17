cmake_minimum_required(VERSION 3.12)

project(cli_tools LANGUAGES CXX)

add_library(cli_tools STATIC
    source/cli_tools.cpp
)

target_include_directories(cli_tools PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(cli_tools PUBLIC cxx_std_17)

add_executable(cli_tools_demo
    tools/cli_tools_demo.cpp
)
target_link_libraries(cli_tools_demo PRIVATE cli_tools)
target_compile_features(cli_tools_demo PRIVATE cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(cli_tools_tests
        tests/test_cli_tools.cpp
    )
    target_link_libraries(cli_tools_tests PRIVATE cli_tools tyst_framework_main)
    add_test(NAME CliToolsTests COMMAND cli_tools_tests)
endif()
