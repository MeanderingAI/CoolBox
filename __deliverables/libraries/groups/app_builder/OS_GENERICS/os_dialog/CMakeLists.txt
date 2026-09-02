cmake_minimum_required(VERSION 3.12)

project(os_dialog LANGUAGES CXX)

add_library(os_dialog STATIC
    source/os_dialog.cpp
)

target_include_directories(os_dialog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(os_dialog PUBLIC cxx_std_17)
target_link_libraries(os_dialog PUBLIC abstract_dialog)

if(WIN32)
    target_link_libraries(os_dialog PRIVATE Comdlg32 Shell32 Ole32)
endif()

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(os_dialog_tests
        tests/test_os_dialog.cpp
    )
    target_link_libraries(os_dialog_tests PRIVATE os_dialog tyst_framework_main)
    add_test(NAME OsDialogTests COMMAND os_dialog_tests)
endif()
