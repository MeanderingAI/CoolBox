cmake_minimum_required(VERSION 3.12)

project(file_browser LANGUAGES CXX)

add_library(file_browser_lib
    source/file_browser.cpp
)

target_include_directories(file_browser_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(file_browser_lib PUBLIC cxx_std_17)
set_target_properties(file_browser_lib PROPERTIES EXPORT_NAME file_browser)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(file_browser_tests
        tests/test_file_browser.cpp
    )
    target_link_libraries(file_browser_tests PRIVATE file_browser_lib tyst_framework_main)
    add_test(NAME FileBrowserTests COMMAND file_browser_tests)
    # Keep a broad filesystem traversal from hanging the entire CTest suite.
    set_tests_properties(FileBrowserTests PROPERTIES TIMEOUT 30)
endif()