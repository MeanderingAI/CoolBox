cmake_minimum_required(VERSION 3.12)
project(full_application_window LANGUAGES CXX)

add_library(full_application_window SHARED
    source/full_application_window.cpp
    source/workspace_dock_host.cpp
)

target_include_directories(full_application_window PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(full_application_window PUBLIC cxx_std_17)
target_link_libraries(full_application_window PUBLIC components windows)

if(WIN32)
    target_link_libraries(full_application_window PRIVATE user32 gdi32 Comdlg32 Shell32 Ole32)
endif()

if(APPLE)
    target_compile_definitions(full_application_window PRIVATE GRAPHICS_HAVE_COCOA_RUNTIME=1)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    if(COCOA_FRAMEWORK)
        target_link_libraries(full_application_window PRIVATE ${COCOA_FRAMEWORK})
    endif()
    if(FOUNDATION_FRAMEWORK)
        target_link_libraries(full_application_window PRIVATE ${FOUNDATION_FRAMEWORK})
    endif()
endif()

if(UNIX AND NOT APPLE)
    find_package(X11 QUIET)
    if(X11_FOUND)
        target_compile_definitions(full_application_window PRIVATE GRAPHICS_HAVE_X11=1)
        target_include_directories(full_application_window PRIVATE ${X11_INCLUDE_DIR})
        target_link_libraries(full_application_window PRIVATE ${X11_LIBRARIES})
    endif()
endif()

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(full_application_window_tests
        tests/test_full_application_window.cpp
    )
    target_link_libraries(full_application_window_tests PRIVATE full_application_window tyst_framework_main)
    if(WIN32)
        add_custom_command(TARGET full_application_window_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:components>
                $<TARGET_FILE:windows>
                $<TARGET_FILE:full_application_window>
                $<TARGET_FILE_DIR:full_application_window_tests>
        )
    endif()
    add_test(NAME FullApplicationWindowTests COMMAND full_application_window_tests)
endif()