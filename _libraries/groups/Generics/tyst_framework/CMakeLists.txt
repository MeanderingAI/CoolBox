cmake_minimum_required(VERSION 3.20)

project(tyst_framework LANGUAGES CXX)

add_library(tyst_framework INTERFACE)
target_include_directories(tyst_framework INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/headers>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(tyst_framework INTERFACE cxx_std_17)

add_library(tyst_framework_main STATIC
    source/tyst_framework_main.cpp
)
target_link_libraries(tyst_framework_main PUBLIC tyst_framework)
target_compile_features(tyst_framework_main PUBLIC cxx_std_17)

# --- CMake package config and export logic ---
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/tyst_framework_main)

install(TARGETS tyst_framework tyst_framework_main
    EXPORT tyst_framework_mainTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY headers/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT tyst_framework_mainTargets
    FILE tyst_framework_mainTargets.cmake
    NAMESPACE tyst_framework_main::
    DESTINATION ${INSTALL_CONFIGDIR}
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/tyst_framework_mainConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/tyst_framework_mainConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/tyst_framework_mainConfig.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)

# Create cmake/tyst_framework_mainConfig.cmake.in template if missing
# Content:
# set_and_check(TYST_FRAMEWORK_MAIN_INCLUDE_DIR "@PACKAGE_INCLUDE_INSTALL_DIR@")
# include("${CMAKE_CURRENT_LIST_DIR}/tyst_framework_mainTargets.cmake")

if(BUILD_TESTING)
    enable_testing()
    add_executable(tyst_framework_tests
        tests/test_tyst_framework.cpp
    )
    target_link_libraries(tyst_framework_tests PRIVATE tyst_framework_main)
    add_test(NAME TystFrameworkTests COMMAND tyst_framework_tests)
endif()