cmake_minimum_required(VERSION 3.12)
project(secret_gen_cli_app LANGUAGES CXX)

add_executable(secret_gen_cli
    src/main.cpp
)

target_link_libraries(secret_gen_cli PRIVATE
    cli_tools
    uuid_generation
)

target_compile_features(secret_gen_cli PRIVATE cxx_std_17)

if(MSVC)
    add_custom_command(TARGET secret_gen_cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:secret_gen_cli>
            $<TARGET_FILE_DIR:secret_gen_cli>
        COMMAND_EXPAND_LISTS
    )
endif()
