cmake_minimum_required(VERSION 3.12)

project(installer_abstraction LANGUAGES CXX)

add_library(installer_abstraction STATIC
    source/installer_abstraction.cpp
)

target_include_directories(installer_abstraction PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(installer_abstraction PUBLIC cxx_std_17)

add_executable(installer_abstraction_notes
    tools/installer_abstraction_notes.cpp
)
target_link_libraries(installer_abstraction_notes PRIVATE installer_abstraction)
target_compile_features(installer_abstraction_notes PRIVATE cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(installer_abstraction_tests
        tests/test_installer_abstraction.cpp
    )
    target_link_libraries(installer_abstraction_tests PRIVATE installer_abstraction tyst_framework_main)
    add_test(NAME InstallerAbstractionTests COMMAND installer_abstraction_tests)
endif()