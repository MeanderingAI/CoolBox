cmake_minimum_required(VERSION 3.12)

project(liquid_ui LANGUAGES CXX)

add_library(liquid_ui_lib
    source/liquid_ui.cpp
)

target_include_directories(liquid_ui_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(liquid_ui_lib PUBLIC full_application_window)
target_compile_features(liquid_ui_lib PUBLIC cxx_std_17)
set_target_properties(liquid_ui_lib PROPERTIES EXPORT_NAME liquid_ui)
