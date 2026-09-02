cmake_minimum_required(VERSION 3.20)
project(database_driver_app LANGUAGES CXX)

add_executable(database_driver
    src/database_driver_main.cpp
)

target_link_libraries(database_driver PRIVATE
    cli_tools
    sql
)

target_compile_features(database_driver PRIVATE cxx_std_17)

add_executable(database_client
    src/database_client_main.cpp
)

add_executable(database_server
    src/database_server_main.cpp
)

target_link_libraries(database_client PRIVATE
    cli_tools
    sql
)

target_link_libraries(database_server PRIVATE
    cli_tools
    sql
    sun_data_structures
    sun_network
)

target_compile_features(database_client PRIVATE cxx_std_17)
target_compile_features(database_server PRIVATE cxx_std_17)

if(MSVC)
    add_custom_command(TARGET database_driver POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:database_driver>
            $<TARGET_FILE_DIR:database_driver>
        COMMAND_EXPAND_LISTS
    )

    add_custom_command(TARGET database_client POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:database_client>
            $<TARGET_FILE_DIR:database_client>
        COMMAND_EXPAND_LISTS
    )

    add_custom_command(TARGET database_server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:database_server>
            $<TARGET_FILE_DIR:database_server>
        COMMAND_EXPAND_LISTS
    )
endif()
