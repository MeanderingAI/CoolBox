cmake_minimum_required(VERSION 3.20)

project(tyst_framework LANGUAGES CXX)




# --- CMake package config and export logic ---
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)


set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/tyst_framework_main)

install(DIRECTORY headers/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})



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

