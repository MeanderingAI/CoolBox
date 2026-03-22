# Option to control Emscripten MODULARIZE (default ON, can override with -DEMSCRIPTEN_MODULARIZE=OFF)
option(EMSCRIPTEN_MODULARIZE "Build Emscripten modules with MODULARIZE=1 (async JS loading)" ON)

cmake_minimum_required(VERSION 3.12)
project(Models LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(COOLBOX_PUBLIC_INCLUDE_ROOT "${CMAKE_SOURCE_DIR}/_libraries/include")
if(EXISTS "${COOLBOX_PUBLIC_INCLUDE_ROOT}")
    set(COOLBOX_HAS_PUBLIC_INCLUDE_ROOT ON)
else()
    set(COOLBOX_HAS_PUBLIC_INCLUDE_ROOT OFF)
endif()

# On MSVC, force a consistent dynamic MSVC runtime to avoid /MT vs /MD mismatches
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL" CACHE STRING "MSVC runtime library (force dynamic debug/runtime)" FORCE)
endif()
if(MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "Force googletest to use the shared MSVC CRT" FORCE)
endif()

# Ensure std::filesystem is available on macOS
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment version")
endif()

# Add _libraries/include as global include directory
if(COOLBOX_HAS_PUBLIC_INCLUDE_ROOT)
    include_directories(${COOLBOX_PUBLIC_INCLUDE_ROOT})
endif()

# Set C++ standard globally
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# On Windows/MSVC, auto-export all symbols so SHARED libs produce import .lib files
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Include the external dependencies from the sub-directory
include(cmake/ExternalDependencies.cmake)

# Pass top-level source dir to gen_docs
set(TOOLBOX_SOURCE_DIR "${CMAKE_SOURCE_DIR}" CACHE PATH "Top-level ToolBox source directory" FORCE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable CTest so add_test() calls in subdirectories are discovered
enable_testing()

# Option to skip building apps/services/demos
option(BUILD_BINARIES "Build binaries (apps, services, demos)" ON)
option(BUILD_BINARIES "Build binaries (apps, services, demos)" ON)
if(BUILD_BINARIES)
    if(EXISTS "${CMAKE_SOURCE_DIR}/_binaries")
        add_subdirectory(_binaries)
    else()
        message(STATUS "BUILD_BINARIES=ON but _binaries directory not found; skipping binaries")
    endif()
endif()

add_subdirectory(_libraries)

# Build apps if present (e.g., apps/lsp)
if(EXISTS "${CMAKE_SOURCE_DIR}/apps")
    add_subdirectory(apps)
endif()

# Option to install and export CoolBox CMake package for external consumers
option(COOLBOX_INSTALL_PACKAGE "Install CoolBox libraries and CMake package config" ON)

if(COOLBOX_INSTALL_PACKAGE)
    function(coolbox_collect_directory_targets dir out_var)
        get_property(dir_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
        set(collected_targets ${dir_targets})

        get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
        foreach(subdir IN LISTS subdirs)
            coolbox_collect_directory_targets("${subdir}" subdir_targets)
            list(APPEND collected_targets ${subdir_targets})
        endforeach()

        set(${out_var} "${collected_targets}" PARENT_SCOPE)
    endfunction()

    function(coolbox_prepare_target_for_export target_name)
        get_target_property(target_source_dir ${target_name} SOURCE_DIR)
        if(NOT target_source_dir)
            return()
        endif()

        set(interface_includes
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        )

        if(COOLBOX_HAS_PUBLIC_INCLUDE_ROOT)
            list(APPEND interface_includes "$<BUILD_INTERFACE:${COOLBOX_PUBLIC_INCLUDE_ROOT}>")
        endif()

        if(EXISTS "${target_source_dir}/headers")
            file(RELATIVE_PATH rel_headers "${CMAKE_SOURCE_DIR}/_libraries" "${target_source_dir}/headers")
            list(APPEND interface_includes
                "$<BUILD_INTERFACE:${target_source_dir}/headers>"
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${rel_headers}>"
            )
        endif()

        get_target_property(existing_interface_includes ${target_name} INTERFACE_INCLUDE_DIRECTORIES)
        if(existing_interface_includes AND NOT existing_interface_includes STREQUAL "existing_interface_includes-NOTFOUND")
            foreach(inc_dir IN LISTS existing_interface_includes)
                if(inc_dir MATCHES "^\\$<BUILD_INTERFACE:.*>$" OR inc_dir MATCHES "^\\$<INSTALL_INTERFACE:.*>$")
                    list(APPEND interface_includes "${inc_dir}")
                    continue()
                endif()

                if(IS_ABSOLUTE "${inc_dir}")
                    if(inc_dir MATCHES "^${CMAKE_SOURCE_DIR}(/|$)" OR inc_dir MATCHES "^${CMAKE_BINARY_DIR}(/|$)")
                        continue()
                    endif()
                endif()

                list(APPEND interface_includes "${inc_dir}")
            endforeach()
        endif()

        list(REMOVE_DUPLICATES interface_includes)
        set_target_properties(${target_name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${interface_includes}")
    endfunction()

    coolbox_collect_directory_targets("${CMAKE_BINARY_DIR}/_libraries" all_targets)
    set(COOLBOX_EXPORT_TARGETS "")
    foreach(target_name IN LISTS all_targets)
        if(NOT TARGET ${target_name})
            continue()
        endif()

        get_target_property(target_imported ${target_name} IMPORTED)
        if(target_imported)
            continue()
        endif()

        if(target_name MATCHES "(^test_|_test$|_tests$|bindings|ml_core)")
            continue()
        endif()

        get_target_property(target_type ${target_name} TYPE)
        if(target_type STREQUAL "STATIC_LIBRARY" OR target_type STREQUAL "SHARED_LIBRARY" OR target_type STREQUAL "INTERFACE_LIBRARY")
            coolbox_prepare_target_for_export(${target_name})
            list(APPEND COOLBOX_EXPORT_TARGETS ${target_name})
        endif()
    endforeach()

    if(COOLBOX_EXPORT_TARGETS)
        message(STATUS "CoolBox package export targets: ${COOLBOX_EXPORT_TARGETS}")
        install(TARGETS ${COOLBOX_EXPORT_TARGETS}
            EXPORT CoolBoxTargets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )

        if(COOLBOX_HAS_PUBLIC_INCLUDE_ROOT)
            install(DIRECTORY ${COOLBOX_PUBLIC_INCLUDE_ROOT}/
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            )
        endif()

        install(DIRECTORY ${CMAKE_SOURCE_DIR}/_libraries/backages/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/backages
            FILES_MATCHING
                PATTERN "*.h"
                PATTERN "*.hpp"
        )

        configure_package_config_file(
            ${CMAKE_SOURCE_DIR}/cmake/CoolBoxConfig.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/CoolBoxConfig.cmake
            INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CoolBox
        )

        write_basic_package_version_file(
            ${CMAKE_CURRENT_BINARY_DIR}/CoolBoxConfigVersion.cmake
            VERSION 1.0.42
            COMPATIBILITY SameMajorVersion
        )

        install(EXPORT CoolBoxTargets
            FILE CoolBoxTargets.cmake
            NAMESPACE CoolBox::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CoolBox
        )

        install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/CoolBoxConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/CoolBoxConfigVersion.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CoolBox
        )
    else()
        message(WARNING "COOLBOX_INSTALL_PACKAGE=ON but no exportable CoolBox library targets were found.")
    endif()
endif()

# Option to build Python bindings
option(BUILD_PYTHON_BINDINGS "Build Python bindings" OFF)

# Add Python bindings if requested
if(BUILD_PYTHON_BINDINGS)
    find_package(pybind11 QUIET)
    if(pybind11_FOUND)
        message(STATUS "Building Python bindings")
        add_subdirectory(python_bindings)
    else()
        message(WARNING "pybind11 not found. Python bindings will not be built.")
        message(STATUS "Install pybind11 with: pip install pybind11[global]")
    endif()
endif()





# QUICHE/HTTP3/Unified server support disabled for pure HTTP2 build
