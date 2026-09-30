cmake_minimum_required(VERSION 3.12)
project(movie_editor_app LANGUAGES CXX)

add_executable(movie_editor
    src/main.cpp
    src/editor_window.cpp
)

target_link_libraries(movie_editor PRIVATE
    movie_editor_core
    full_application_window
    components
    charts
    os_dialog
    cli_tools
)

target_compile_features(movie_editor PRIVATE cxx_std_17)

if(MSVC)
    add_custom_command(TARGET movie_editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:movie_editor>
            $<TARGET_FILE_DIR:movie_editor>
        COMMAND_EXPAND_LISTS
    )
endif()
