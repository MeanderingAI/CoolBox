cmake_minimum_required(VERSION 3.16)

add_library(movie_editor_core SHARED
    source/media_bin.cpp
    source/editor_project.cpp
    source/render.cpp
    source/export.cpp
    source/project_io.cpp
)

target_compile_features(movie_editor_core PUBLIC cxx_std_17)
target_include_directories(movie_editor_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(movie_editor_core PUBLIC
    video_codec
    video_container
    video_timeline
    audio_processing
    audio_container
    json
)
if(TARGET graphics_textures)
    target_link_libraries(movie_editor_core PUBLIC graphics_textures)
endif()

# std::filesystem requires linking stdc++fs on some older GCC/libstdc++
# versions (GCC < 9); harmless to add unconditionally on others.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(movie_editor_core PUBLIC stdc++fs)
endif()

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(movie_editor_core_tests tests/test_movie_editor_core.cpp)
    target_link_libraries(movie_editor_core_tests PRIVATE movie_editor_core charts tyst_framework_main)
    add_test(NAME MovieEditorCoreTests COMMAND movie_editor_core_tests)
endif()
