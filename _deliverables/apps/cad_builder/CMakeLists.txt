cmake_minimum_required(VERSION 3.12)
project(cad_builder_app LANGUAGES CXX)

add_executable(cad_builder
    src/cad_builder_window.cpp
    src/main.cpp
)

target_link_libraries(cad_builder PRIVATE
    full_application_window
    components
    charts
    os_dialog
    xml_screen_descriptor_parser
    cad_file_processing
)

if(TARGET OpenGL::GL)
    target_link_libraries(cad_builder PRIVATE OpenGL::GL)
endif()

if(TARGET ai_chat_lib)
    target_link_libraries(cad_builder PRIVATE ai_chat_lib)
endif()

target_compile_features(cad_builder PRIVATE cxx_std_17)

if(MSVC)
    add_custom_command(TARGET cad_builder POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:cad_builder>
            $<TARGET_FILE_DIR:cad_builder>
        COMMAND_EXPAND_LISTS
    )
endif()
