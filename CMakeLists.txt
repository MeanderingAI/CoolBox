
cmake_minimum_required(VERSION 3.20)
project(CoolBoxSQL LANGUAGES C CXX)

# Force Eigen to use at least C++11 (and respect top-level C++ standard)
set(EIGEN_TEST_CXX11 ON CACHE BOOL "Enable C++11/17 for Eigen" FORCE)


# Ensure custom Find*.cmake modules are found before dependency checks run.
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake")

include(cmake/FindAllDependencies.cmake)

# Option to control Emscripten MODULARIZE (default ON, can override with -DEMSCRIPTEN_MODULARIZE=OFF)
option(EMSCRIPTEN_MODULARIZE "Build Emscripten modules with MODULARIZE=1 (async JS loading)" ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(COOLBOX_PUBLIC_INCLUDE_ROOT "${CMAKE_SOURCE_DIR}/_deliverables/libraries/include")
if(EXISTS "${COOLBOX_PUBLIC_INCLUDE_ROOT}")
    set(COOLBOX_HAS_PUBLIC_INCLUDE_ROOT ON)
else()
    set(COOLBOX_HAS_PUBLIC_INCLUDE_ROOT OFF)
endif()

# On MSVC, force a consistent dynamic MSVC runtime to avoid /MT vs /MD mismatches
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL" CACHE STRING "MSVC runtime library (force dynamic debug/runtime)" FORCE)
    # /FS: force synchronous PDB writes to fix C1041 when multiple CL.EXE run in parallel
    add_compile_options(/FS)
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

option(COOLBOX_ENABLE_GPU_ACCELERATION "Enable GPU backend detection and acceleration compile definitions" ON)

set(COOLBOX_GPU_BACKENDS "")
set(COOLBOX_GPU_ENABLED OFF)
if(COOLBOX_ENABLE_GPU_ACCELERATION)
    find_package(CUDAToolkit QUIET)
    if(CUDAToolkit_FOUND)
        set(COOLBOX_HAS_CUDA ON)
        set(COOLBOX_GPU_ENABLED ON)
        list(APPEND COOLBOX_GPU_BACKENDS "CUDA")
    else()
        set(COOLBOX_HAS_CUDA OFF)
    endif()

    find_package(OpenCL QUIET)
    if(OpenCL_FOUND)
        set(COOLBOX_HAS_OPENCL ON)
        set(COOLBOX_GPU_ENABLED ON)
        list(APPEND COOLBOX_GPU_BACKENDS "OpenCL")
    else()
        set(COOLBOX_HAS_OPENCL OFF)
    endif()

    find_package(OpenGL QUIET)
    if(OpenGL_FOUND)
        set(COOLBOX_HAS_OPENGL ON)
        set(COOLBOX_GPU_ENABLED ON)
        list(APPEND COOLBOX_GPU_BACKENDS "OpenGL")
    else()
        set(COOLBOX_HAS_OPENGL OFF)
    endif()

    if(COOLBOX_GPU_ENABLED)
        list(REMOVE_DUPLICATES COOLBOX_GPU_BACKENDS)
        string(JOIN ", " COOLBOX_GPU_BACKENDS_STR ${COOLBOX_GPU_BACKENDS})
        message(STATUS "GPU acceleration backends detected: ${COOLBOX_GPU_BACKENDS_STR}")
    else()
        message(STATUS "GPU acceleration requested, but no CUDA/OpenCL/OpenGL backend was detected.")
    endif()

    add_compile_definitions(
        COOLBOX_ENABLE_GPU_ACCELERATION=1
        $<$<BOOL:${COOLBOX_HAS_CUDA}>:COOLBOX_HAS_CUDA=1>
        $<$<BOOL:${COOLBOX_HAS_OPENCL}>:COOLBOX_HAS_OPENCL=1>
        $<$<BOOL:${COOLBOX_HAS_OPENGL}>:COOLBOX_HAS_OPENGL=1>
    )
else()
    message(STATUS "GPU acceleration detection disabled (COOLBOX_ENABLE_GPU_ACCELERATION=OFF).")
endif()

# Enable CTest so add_test() calls in subdirectories are discovered
enable_testing()

function(_coolbox_collect_runtime_dependency_targets target out_var)
    get_property(visited_targets GLOBAL PROPERTY COOLBOX_RUNTIME_DEP_VISITED)
    if(NOT visited_targets)
        set(visited_targets "")
    endif()

    list(FIND visited_targets "${target}" target_index)
    if(NOT target_index EQUAL -1)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    list(APPEND visited_targets "${target}")
    set_property(GLOBAL PROPERTY COOLBOX_RUNTIME_DEP_VISITED "${visited_targets}")

    set(runtime_targets "")
    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "SHARED_LIBRARY" OR target_type STREQUAL "MODULE_LIBRARY")
        list(APPEND runtime_targets "${target}")
    endif()

    foreach(link_property IN ITEMS LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
        get_target_property(linked_items "${target}" ${link_property})
        if(linked_items AND NOT linked_items STREQUAL "linked_items-NOTFOUND")
            foreach(linked_item IN LISTS linked_items)
                if(TARGET "${linked_item}")
                    _coolbox_collect_runtime_dependency_targets("${linked_item}" dependency_targets)
                    list(APPEND runtime_targets ${dependency_targets})
                endif()
            endforeach()
        endif()
    endforeach()

    list(REMOVE_DUPLICATES runtime_targets)
    set(${out_var} "${runtime_targets}" PARENT_SCOPE)
endfunction()

function(coolbox_stage_test_runtime_dependencies test_target)
    if(NOT WIN32 OR NOT TARGET "${test_target}")
        return()
    endif()

    set_property(GLOBAL PROPERTY COOLBOX_RUNTIME_DEP_VISITED "")
    _coolbox_collect_runtime_dependency_targets("${test_target}" runtime_targets)
    list(REMOVE_ITEM runtime_targets "${test_target}")
    list(REMOVE_DUPLICATES runtime_targets)

    if(runtime_targets)
        set(copy_sources "")
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
            set(copy_sources "$<TARGET_RUNTIME_DLLS:${test_target}>")
        else()
            foreach(runtime_target IN LISTS runtime_targets)
                list(APPEND copy_sources "$<TARGET_FILE:${runtime_target}>")
            endforeach()
        endif()

        add_custom_command(TARGET ${test_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${copy_sources}
                $<TARGET_FILE_DIR:${test_target}>
            COMMAND_EXPAND_LISTS
        )
    endif()
endfunction()

# Option to skip building apps/services/demos
option(BUILD_BINARIES "Build binaries (apps, services, demos)" ON)
option(BUILD_BINARIES "Build binaries (apps, services, demos)" ON)
option(BUILD_PRODUCT_INSTALLER_ABSTRACTIONS "Build installer abstraction helpers for release-packaged products" OFF)
if(BUILD_BINARIES)
    if(EXISTS "${CMAKE_SOURCE_DIR}/_binaries")
        add_subdirectory(_binaries)
    else()
        message(STATUS "BUILD_BINARIES=ON but _binaries directory not found; skipping binaries")
    endif()
endif()

add_subdirectory(_deliverables/libraries)

# Build apps if present (e.g., apps/lsp)
if(EXISTS "${CMAKE_SOURCE_DIR}/_deliverables/apps")
    add_subdirectory(_deliverables/apps)
endif()

option(BUILD_PRODUCTS "Build products under _Product" ON)
if(BUILD_PRODUCTS AND EXISTS "${CMAKE_SOURCE_DIR}/_deliverables/Product")
    add_subdirectory(_deliverables/Product)
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
            file(RELATIVE_PATH rel_headers "${CMAKE_SOURCE_DIR}/_deliverables/libraries" "${target_source_dir}/headers")
            list(APPEND interface_includes
                "$<BUILD_INTERFACE:${target_source_dir}/headers>"
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${rel_headers}>"
            )
        endif()

        # Some targets use an "include" subdirectory instead of "headers"
        # for their public headers; honor that convention too so their
        # INTERFACE_INCLUDE_DIRECTORIES aren't silently dropped below.
        if(EXISTS "${target_source_dir}/include")
            file(RELATIVE_PATH rel_include "${CMAKE_SOURCE_DIR}/_deliverables/libraries" "${target_source_dir}/include")
            list(APPEND interface_includes
                "$<BUILD_INTERFACE:${target_source_dir}/include>"
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${rel_include}>"
            )
        endif()

        get_target_property(existing_interface_includes ${target_name} INTERFACE_INCLUDE_DIRECTORIES)
        if(existing_interface_includes AND NOT existing_interface_includes STREQUAL "existing_interface_includes-NOTFOUND")
            foreach(inc_dir IN LISTS existing_interface_includes)
                if(inc_dir MATCHES "^\\$<BUILD_INTERFACE:.*>$" OR inc_dir MATCHES "^\\$<INSTALL_INTERFACE:.*>$")
                    list(APPEND interface_includes "${inc_dir}")
                    continue()
                endif()

                # Plain absolute paths that point elsewhere in this source
                # or build tree (e.g. a sibling package's "include" dir
                # pulled in via target_include_directories) aren't valid
                # once installed, but they're still needed to build targets
                # (like tests) directly against this target's build tree.
                # Wrap them in BUILD_INTERFACE instead of dropping them so
                # in-tree consumers keep working; they're simply omitted
                # from the installed/export interface.
                if(IS_ABSOLUTE "${inc_dir}")
                    if(inc_dir MATCHES "^${CMAKE_SOURCE_DIR}(/|$)" OR inc_dir MATCHES "^${CMAKE_BINARY_DIR}(/|$)")
                        list(APPEND interface_includes "$<BUILD_INTERFACE:${inc_dir}>")
                        continue()
                    endif()
                endif()

                list(APPEND interface_includes "${inc_dir}")
            endforeach()
        endif()

        list(REMOVE_DUPLICATES interface_includes)
        set_target_properties(${target_name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${interface_includes}")
    endfunction()

    coolbox_collect_directory_targets("${CMAKE_BINARY_DIR}/_deliverables/libraries" all_targets)
    set(COOLBOX_EXPORT_TARGETS "")
    foreach(target_name IN LISTS all_targets)
        if(NOT TARGET ${target_name})
            continue()
        endif()

        get_target_property(target_imported ${target_name} IMPORTED)
        if(target_imported)
            continue()
        endif()

        if(target_name MATCHES "(^test_|_test$|_tests$|bindings|ml_core|^tyst_framework$|^tyst_framework_main$)")
            continue()
        endif()

        get_target_property(target_type ${target_name} TYPE)
        if(target_type STREQUAL "STATIC_LIBRARY" OR target_type STREQUAL "SHARED_LIBRARY" OR target_type STREQUAL "INTERFACE_LIBRARY")
            coolbox_prepare_target_for_export(${target_name})
            list(APPEND COOLBOX_EXPORT_TARGETS ${target_name})
        endif()
    endforeach()

    if(COOLBOX_EXPORT_TARGETS)
        # Ensure we don't try to install the same target more than once
        list(REMOVE_DUPLICATES COOLBOX_EXPORT_TARGETS)
        message(STATUS "CoolBox package export targets: ${COOLBOX_EXPORT_TARGETS}")
        install(TARGETS ${COOLBOX_EXPORT_TARGETS}
            EXPORT CoolBoxTargets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )

        if(COOLBOX_HAS_PUBLIC_INCLUDE_ROOT)
            install(DIRECTORY ${COOLBOX_PUBLIC_INCLUDE_ROOT}/
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            )
        endif()

        install(DIRECTORY ${CMAKE_SOURCE_DIR}/_deliverables/libraries/packages/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/packages
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
        add_subdirectory(_deliverables/libraries/bindings/python_bindings)
    else()
        message(WARNING "pybind11 not found. Python bindings will not be built.")
        message(STATUS "Install pybind11 with: pip install pybind11[global]")
    endif()
endif()

# Find and include Eigen3 globally for all targets that need it.
# ExternalDependencies.cmake already provides Eigen3::Eigen in CI/toolchains
# where a package config is unavailable.
if(NOT TARGET Eigen3::Eigen)
    find_package(Eigen3 QUIET CONFIG)
    if(NOT TARGET Eigen3::Eigen)
        find_package(Eigen3 REQUIRED)
    endif()
endif()
if(DEFINED EIGEN3_INCLUDE_DIR)
    include_directories(${EIGEN3_INCLUDE_DIR})
endif()

# External QUIC/HTTP3 dependency support disabled for pure HTTP2 build
