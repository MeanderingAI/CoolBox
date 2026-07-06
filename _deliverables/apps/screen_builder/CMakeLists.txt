cmake_minimum_required(VERSION 3.12)
project(screen_builder LANGUAGES CXX)

add_executable(screen_builder
    src/main.cpp
)

target_link_libraries(screen_builder PRIVATE
    full_application_window
    components
    file_editor_lib
    ios_chrome_lib
    xml_screen_descriptor_parser
)

target_compile_features(screen_builder PRIVATE cxx_std_17)

if(MSVC)
    add_custom_command(TARGET screen_builder POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:screen_builder>
            $<TARGET_FILE_DIR:screen_builder>
        COMMAND_EXPAND_LISTS
    )
endif()
