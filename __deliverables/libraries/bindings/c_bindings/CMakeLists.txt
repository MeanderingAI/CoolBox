cmake_minimum_required(VERSION 3.16)
project(CoolBoxCBindings LANGUAGES C CXX)

include(CTest)
include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT COMMAND coolbox_stage_test_runtime_dependencies)
    function(_coolbox_collect_runtime_dependency_targets target out_var)
        if(NOT TARGET "${target}")
            set(${out_var} "" PARENT_SCOPE)
            return()
        endif()

        get_property(visited GLOBAL PROPERTY COOLBOX_RUNTIME_DEP_VISITED)
        if(NOT visited)
            set(visited "")
        endif()

        if("${target}" IN_LIST visited)
            set(${out_var} "" PARENT_SCOPE)
            return()
        endif()

        set_property(GLOBAL APPEND PROPERTY COOLBOX_RUNTIME_DEP_VISITED "${target}")
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
            foreach(runtime_target IN LISTS runtime_targets)
                list(APPEND copy_sources "$<TARGET_FILE:${runtime_target}>")
            endforeach()

            add_custom_command(TARGET ${test_target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${copy_sources}
                    $<TARGET_FILE_DIR:${test_target}>
                COMMAND_EXPAND_LISTS
            )
        endif()
    endfunction()
endif()


# Always add the correct metadata_management subdirectory for this repo layout
if(NOT TARGET metadata_management)
    add_subdirectory(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../groups/trekker/MISC/metadata_management
        ${CMAKE_CURRENT_BINARY_DIR}/metadata_management
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../groups/trekker/MISC/uuid_generation" AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../groups/trekker/MISC/uuid_generation")
    if(NOT TARGET uuid_generation)
        add_subdirectory(
                ${CMAKE_CURRENT_SOURCE_DIR}/../../groups/trekker/MISC/uuid_generation
                ${CMAKE_CURRENT_BINARY_DIR}/uuid_generation
        )
    endif()
endif()

add_library(coolbox_c_bindings SHARED
    src/coolbox_c.c
    src/uuid_generation_c_bridge.cpp
)

add_library(CoolBox::c_bindings ALIAS coolbox_c_bindings)

set(COOLBOX_C_BINDINGS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

set_target_properties(coolbox_c_bindings PROPERTIES
    OUTPUT_NAME coolbox_c_bindings
    LINKER_LANGUAGE CXX
    POSITION_INDEPENDENT_CODE ON
    ARCHIVE_OUTPUT_DIRECTORY "${COOLBOX_C_BINDINGS_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${COOLBOX_C_BINDINGS_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${COOLBOX_C_BINDINGS_OUTPUT_DIR}"
)

foreach(config IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${config}" config_upper)
    set_target_properties(coolbox_c_bindings PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY_${config_upper} "${COOLBOX_C_BINDINGS_OUTPUT_DIR}/${config}"
        LIBRARY_OUTPUT_DIRECTORY_${config_upper} "${COOLBOX_C_BINDINGS_OUTPUT_DIR}/${config}"
        RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${COOLBOX_C_BINDINGS_OUTPUT_DIR}/${config}"
    )
endforeach()

target_compile_definitions(coolbox_c_bindings
    PRIVATE
        COOLBOX_C_BINDINGS_EXPORTS
)

if(WIN32)
    target_compile_definitions(coolbox_c_bindings
        PUBLIC
            COOLBOX_C_BINDINGS_SHARED
    )
    set_target_properties(coolbox_c_bindings PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
endif()

target_include_directories(coolbox_c_bindings
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../groups/trekker/MISC/uuid_generation/headers
)

target_link_libraries(coolbox_c_bindings
    PRIVATE
        metadata_management
    uuid_generation
)

# Only link math library on non-Windows platforms
if(NOT WIN32)
    target_link_libraries(coolbox_c_bindings PUBLIC m)
endif()

install(TARGETS coolbox_c_bindings
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(BUILD_TESTING)
    add_executable(test_coolbox_c_bindings
        tests/test_coolbox_c_bindings.c
    )
    target_link_libraries(test_coolbox_c_bindings PRIVATE coolbox_c_bindings)
    coolbox_stage_test_runtime_dependencies(test_coolbox_c_bindings)
    add_test(NAME test_coolbox_c_bindings COMMAND test_coolbox_c_bindings)
endif()
