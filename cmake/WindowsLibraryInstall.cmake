set(_coolbox_dll_targets)
set(_coolbox_dll_manifest "")
foreach(_target IN LISTS all_targets)
    get_target_property(_type "${_target}" TYPE)
    if(_type STREQUAL "SHARED_LIBRARY" OR _type STREQUAL "MODULE_LIBRARY")
        list(APPEND _coolbox_dll_targets "${_target}")
        string(APPEND _coolbox_dll_manifest
            "$<TARGET_FILE:${_target}>\n"
            "$<JOIN:$<TARGET_RUNTIME_DLLS:${_target}>,\n>\n"
        )
    endif()
endforeach()

add_custom_target(coolbox_windows_libraries DEPENDS ${_coolbox_dll_targets})
file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/coolbox-windows-dlls-$<CONFIG>.txt"
    CONTENT "${_coolbox_dll_manifest}"
)
list(LENGTH _coolbox_dll_targets _coolbox_dll_count)
message(STATUS "Windows DLL install: ${_coolbox_dll_count} shared/module library targets")
