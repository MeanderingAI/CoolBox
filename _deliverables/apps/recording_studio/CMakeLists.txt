cmake_minimum_required(VERSION 3.12)
project(recording_studio_app LANGUAGES CXX)

add_executable(recording_studio
    src/main.cpp
)

target_link_libraries(recording_studio PRIVATE
    full_application_window
    cli_tools
    ios_chrome_lib
)

target_compile_features(recording_studio PRIVATE cxx_std_17)

if(APPLE)
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
    if(COREAUDIO_FRAMEWORK)
        target_link_libraries(recording_studio PRIVATE ${COREAUDIO_FRAMEWORK})
    endif()
    if(AUDIOTOOLBOX_FRAMEWORK)
        target_link_libraries(recording_studio PRIVATE ${AUDIOTOOLBOX_FRAMEWORK})
    endif()
endif()

if(MSVC)
    add_custom_command(TARGET recording_studio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:recording_studio>
            $<TARGET_FILE_DIR:recording_studio>
        COMMAND_EXPAND_LISTS
    )
endif()
