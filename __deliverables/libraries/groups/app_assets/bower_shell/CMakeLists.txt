cmake_minimum_required(VERSION 3.12)

project(bower_shell LANGUAGES CXX)

add_library(bower_shell_lib
    source/bower_shell.cpp
)

target_include_directories(bower_shell_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(bower_shell_lib PUBLIC cxx_std_17)
set_target_properties(bower_shell_lib PROPERTIES EXPORT_NAME bower_shell)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(bower_shell_tests
        tests/test_bower_shell.cpp
    )
    target_link_libraries(bower_shell_tests PRIVATE bower_shell_lib tyst_framework_main)
    add_test(NAME BowerShellTests COMMAND bower_shell_tests)
endif()