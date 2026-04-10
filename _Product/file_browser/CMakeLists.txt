cmake_minimum_required(VERSION 3.15)
project(file_browser_app LANGUAGES CXX)

add_executable(file_browser
    src/main.cpp
    src/file_browser_app.cpp
)

target_compile_features(file_browser PRIVATE cxx_std_17)
target_include_directories(file_browser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(file_browser PRIVATE full_application_window file_browser_lib components)

if(TARGET installer_abstraction)
    target_link_libraries(file_browser PRIVATE installer_abstraction)
    target_compile_definitions(file_browser PRIVATE COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS=1)
endif()

if(WIN32)
    if(TARGET full_application_window)
        add_custom_command(TARGET file_browser POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:full_application_window>
                $<TARGET_FILE_DIR:file_browser>
        )
    endif()
    if(TARGET components)
        add_custom_command(TARGET file_browser POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:components>
                $<TARGET_FILE_DIR:file_browser>
        )
    endif()
    if(TARGET json)
        add_custom_command(TARGET file_browser POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:json>
                $<TARGET_FILE_DIR:file_browser>
        )
    endif()
endif()