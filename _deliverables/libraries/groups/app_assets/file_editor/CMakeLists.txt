cmake_minimum_required(VERSION 3.12)

project(file_editor LANGUAGES CXX)

add_library(file_editor_lib
    source/file_editor.cpp
)

target_include_directories(file_editor_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(file_editor_lib PUBLIC cxx_std_17)
set_target_properties(file_editor_lib PROPERTIES EXPORT_NAME file_editor)
