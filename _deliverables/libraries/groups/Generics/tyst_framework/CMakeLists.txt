cmake_minimum_required(VERSION 3.20)

project(tyst_framework LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

add_library(tyst_framework_main STATIC
    ${CMAKE_SOURCE_DIR}/_tyst_main/tyst_framework_main.cpp
)
target_include_directories(tyst_framework_main PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/headers>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(tyst_framework_main PUBLIC cxx_std_17)

# --- CMake package config and export logic ---
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/tyst_framework_main)

install(DIRECTORY headers/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS tyst_framework_main
    EXPORT tyst_framework_mainTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(EXPORT tyst_framework_mainTargets
    FILE tyst_framework_mainTargets.cmake
    NAMESPACE tyst::
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
