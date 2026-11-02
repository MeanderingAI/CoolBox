cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED TEST_ROOT OR "${TEST_ROOT}" STREQUAL "" OR EXISTS "${TEST_ROOT}")
    message(FATAL_ERROR "Specify a new, empty test directory with -DTEST_ROOT.")
endif()
file(MAKE_DIRECTORY "${TEST_ROOT}/Release" "${TEST_ROOT}/Debug" "${TEST_ROOT}/other")
set(_installer "${CMAKE_CURRENT_LIST_DIR}/../InstallWindowsDlls.cmake")

function(check_install name manifest expected_result expected_message)
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            "-DCOOLBOX_DLL_MANIFEST=${manifest}"
            "-DCOOLBOX_DLL_DESTINATION=${TEST_ROOT}/${name}"
            -P "${_installer}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if(expected_result STREQUAL "success")
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "${name} failed: ${output}\n${error}")
        endif()
    elseif(result EQUAL 0)
        message(FATAL_ERROR "${name} unexpectedly succeeded.")
    endif()
    if(NOT "${output}\n${error}" MATCHES "${expected_message}")
        message(FATAL_ERROR "${name} did not report '${expected_message}': ${output}\n${error}")
    endif()
endfunction()

file(WRITE "${TEST_ROOT}/Release/library with spaces.dll" "Release")
file(WRITE "${TEST_ROOT}/Debug/library with spaces.dll" "Debug")
file(WRITE "${TEST_ROOT}/Debug/stale.dll" "stale")
file(WRITE "${TEST_ROOT}/Release/dependency.dll" "dependency")
set(_manifest "${TEST_ROOT}/dlls.txt")
file(WRITE "${_manifest}"
    "${TEST_ROOT}/Release/library with spaces.dll\n"
    "${TEST_ROOT}/Release/dependency.dll\n"
    "${TEST_ROOT}/Release/dependency.dll\n\n"
)
file(MAKE_DIRECTORY "${TEST_ROOT}/success")
file(WRITE "${TEST_ROOT}/success/library with spaces.dll" "old Debug")
file(WRITE "${TEST_ROOT}/success/unrelated.dll" "unrelated")
check_install(success "${_manifest}" success "Installed 2 DLLs")
file(READ "${TEST_ROOT}/success/library with spaces.dll" _content)
if(NOT _content STREQUAL "Release" OR EXISTS "${TEST_ROOT}/success/stale.dll")
    message(FATAL_ERROR "Installation selected Debug/stale files instead of the manifest outputs.")
endif()
file(READ "${TEST_ROOT}/success/dependency.dll" _content)
if(NOT _content STREQUAL "dependency")
    message(FATAL_ERROR "Runtime dependency was not installed.")
endif()
file(READ "${TEST_ROOT}/success/unrelated.dll" _content)
if(NOT _content STREQUAL "unrelated")
    message(FATAL_ERROR "Installation changed an unrelated existing file.")
endif()
check_install(success "${_manifest}" success "Installed 2 DLLs")

check_install(no_manifest "${TEST_ROOT}/absent.txt" failure "DLL manifest not found")
file(WRITE "${_manifest}" "")
check_install(empty_manifest "${_manifest}" failure "No DLLs are configured")

file(WRITE "${_manifest}"
    "${TEST_ROOT}/Release/dependency.dll\n"
    "${TEST_ROOT}/Release/missing.dll\n"
)
check_install(missing_dll "${_manifest}" failure "Required DLL is missing")
if(EXISTS "${TEST_ROOT}/missing_dll")
    message(FATAL_ERROR "Missing DLL caused a partial installation.")
endif()

file(WRITE "${TEST_ROOT}/other/DEPENDENCY.dll" "conflicting dependency")
file(WRITE "${_manifest}"
    "${TEST_ROOT}/Release/dependency.dll\n"
    "${TEST_ROOT}/other/DEPENDENCY.dll\n"
)
check_install(conflicting_names "${_manifest}" failure "same installation name")
if(EXISTS "${TEST_ROOT}/conflicting_names")
    message(FATAL_ERROR "Conflicting DLLs caused a partial installation.")
endif()

file(WRITE "${_manifest}" "${TEST_ROOT}/Release/dependency.dll\n")
file(WRITE "${TEST_ROOT}/blocked_destination" "not a directory")
check_install(blocked_destination "${_manifest}" failure "")
message(STATUS "Windows DLL install regression checks passed.")
