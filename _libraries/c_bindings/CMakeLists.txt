cmake_minimum_required(VERSION 3.16)
project(CoolBoxCBindings LANGUAGES C)

include(CTest)
include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(coolbox_c_bindings STATIC
    src/coolbox_c.c
)

add_library(CoolBox::c_bindings ALIAS coolbox_c_bindings)

set_target_properties(coolbox_c_bindings PROPERTIES
    OUTPUT_NAME coolbox_c_bindings
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(coolbox_c_bindings
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

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
    add_test(NAME test_coolbox_c_bindings COMMAND test_coolbox_c_bindings)
endif()
