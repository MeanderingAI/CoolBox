cmake_minimum_required(VERSION 3.12)
project(movie_editor_app LANGUAGES CXX)

set(MOVIE_EDITOR_VERSION "0.1.0")

# Best-effort short git commit hash for precise "which build is this"
# diagnostics (falls back to "unknown" outside a git checkout, e.g. a
# source tarball).
find_package(Git QUIET)
set(MOVIE_EDITOR_GIT_HASH "unknown")
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE MOVIE_EDITOR_GIT_HASH_OUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE MOVIE_EDITOR_GIT_RESULT
    )
    if(MOVIE_EDITOR_GIT_RESULT EQUAL 0 AND MOVIE_EDITOR_GIT_HASH_OUT)
        set(MOVIE_EDITOR_GIT_HASH "${MOVIE_EDITOR_GIT_HASH_OUT}")
    endif()
endif()

add_executable(movie_editor
    src/main.cpp
    src/editor_window.cpp
)

target_compile_definitions(movie_editor PRIVATE
    MOVIE_EDITOR_VERSION="${MOVIE_EDITOR_VERSION}"
    MOVIE_EDITOR_GIT_HASH="${MOVIE_EDITOR_GIT_HASH}"
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
