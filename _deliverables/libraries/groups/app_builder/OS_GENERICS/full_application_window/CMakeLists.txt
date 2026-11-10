cmake_minimum_required(VERSION 3.13)
project(full_application_window LANGUAGES CXX)

add_library(full_application_window SHARED
    source/full_application_window.cpp
    source/workspace_dock_host.cpp
    ../../GRAPHICS/os_generic/source/native_menu.cpp
)

target_include_directories(full_application_window PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_include_directories(full_application_window PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../GRAPHICS/os_generic/headers)
target_compile_features(full_application_window PUBLIC cxx_std_17)
target_link_libraries(full_application_window PUBLIC components windows)

add_library(graphics_os_generic INTERFACE)
add_library(graphics::os_generic ALIAS graphics_os_generic)
target_link_libraries(graphics_os_generic INTERFACE full_application_window)
if(TARGET graphics)
    target_link_libraries(graphics INTERFACE graphics_os_generic)
endif()

if(WIN32)
    set_target_properties(full_application_window PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
    target_link_libraries(full_application_window PRIVATE user32 gdi32 Comdlg32 Shell32 Ole32 opengl32)
endif()

if(APPLE)
    # Objective-C++ language and standard properties require CMake 3.16.
    if(CMAKE_VERSION VERSION_LESS 3.16)
        message(FATAL_ERROR "The Cocoa native menu backend requires CMake 3.16 or newer.")
    endif()
    enable_language(OBJCXX)
    set_target_properties(full_application_window PROPERTIES OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED YES)
    target_sources(full_application_window PRIVATE source/cocoa_native_menu.mm)
    set_source_files_properties(source/cocoa_native_menu.mm PROPERTIES COMPILE_OPTIONS "-fno-objc-arc")
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
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GTK3 QUIET IMPORTED_TARGET "gtk+-3.0>=3.20" "gdk-x11-3.0>=3.20")
    endif()
    if(GTK3_FOUND AND X11_FOUND)
        target_compile_definitions(full_application_window PRIVATE GRAPHICS_HAVE_GTK3=1)
        target_link_libraries(full_application_window PRIVATE PkgConfig::GTK3)
    elseif(COOLBOX_MOVIE_EDITOR_ONLY)
        message(FATAL_ERROR "The Linux movie editor requires GTK3/X11 development packages (libgtk-3-dev, libx11-dev and pkg-config on Debian/Ubuntu).")
    else()
        message(WARNING "GTK3 development packages not found; native Linux menu bars are unavailable (install libgtk-3-dev and pkg-config).")
    endif()
    if(X11_FOUND)
        target_compile_definitions(full_application_window PRIVATE GRAPHICS_HAVE_X11=1)
        target_include_directories(full_application_window PRIVATE ${X11_INCLUDE_DIR})
        target_link_libraries(full_application_window PRIVATE ${X11_LIBRARIES})
    else()
        message(WARNING
            "X11 development headers not found (libx11-dev package on Debian/Ubuntu, "
            "libX11-devel on Fedora). full_application_window will fall back to a "
            "Headless backend: windows will report success from create() but no "
            "window/content will ever actually appear on screen. Install libx11-dev "
            "and reconfigure to enable real GUI windows on Linux.")
    endif()
endif()

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(full_application_window_tests
        tests/test_full_application_window.cpp
        tests/test_native_menu.cpp
    )
    target_link_libraries(full_application_window_tests PRIVATE full_application_window tyst_framework_main)
    if(UNIX AND NOT APPLE AND GTK3_FOUND AND X11_FOUND)
        target_compile_definitions(full_application_window_tests PRIVATE GRAPHICS_TEST_GTK3=1)
        target_link_libraries(full_application_window_tests PRIVATE PkgConfig::GTK3 ${X11_LIBRARIES})
        target_include_directories(full_application_window_tests PRIVATE ${X11_INCLUDE_DIR})
    endif()
    coolbox_stage_test_runtime_dependencies(full_application_window_tests)
    add_test(NAME FullApplicationWindowTests COMMAND full_application_window_tests)

    add_executable(plotting_tests tests/test_plotting.cpp)
    target_link_libraries(plotting_tests PRIVATE full_application_window tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(plotting_tests)
    add_test(NAME PlottingTests COMMAND plotting_tests)
endif()