cmake_minimum_required(VERSION 3.20)
project(database_app LANGUAGES CXX)

add_executable(database_app
    src/main.cpp
)

target_link_libraries(database_app PRIVATE
    metadata_management
    sql_parser
    data_structures
)

target_include_directories(database_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/groups/trekker/DATASTRUCTURE/trees/headers
)

target_compile_features(database_app PRIVATE cxx_std_17)

if(MSVC)
    add_custom_command(TARGET database_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:database_app>
            $<TARGET_FILE_DIR:database_app>
        COMMAND_EXPAND_LISTS
    )
endif()
