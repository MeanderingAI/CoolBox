cmake_minimum_required(VERSION 3.12)

project(abstract_dialog LANGUAGES CXX)

add_library(abstract_dialog STATIC
    source/abstract_dialog.cpp
)

target_include_directories(abstract_dialog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(abstract_dialog PUBLIC cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(abstract_dialog_tests
        tests/test_abstract_dialog.cpp
    )
    target_link_libraries(abstract_dialog_tests PRIVATE abstract_dialog tyst_framework_main)
    add_test(NAME AbstractDialogTests COMMAND abstract_dialog_tests)
endif()
