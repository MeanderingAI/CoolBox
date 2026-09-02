cmake_minimum_required(VERSION 3.15)
project(bower_shell_app LANGUAGES CXX)

add_executable(bower_shell
    src/main.cpp
)

target_compile_features(bower_shell PRIVATE cxx_std_17)
target_include_directories(bower_shell PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(bower_shell PRIVATE bower_shell_lib)

if(TARGET installer_abstraction)
    target_link_libraries(bower_shell PRIVATE installer_abstraction)
    target_compile_definitions(bower_shell PRIVATE COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS=1)
endif()