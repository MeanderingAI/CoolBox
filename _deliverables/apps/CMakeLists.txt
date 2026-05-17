cmake_minimum_required(VERSION 3.12)

# Add individual apps if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp/CMakeLists.txt")
    add_subdirectory(lsp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/distribution_tag/CMakeLists.txt")
    add_subdirectory(distribution_tag)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/distribution_storage/CMakeLists.txt")
    add_subdirectory(distribution_storage)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/database/CMakeLists.txt")
    add_subdirectory(database)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/secret_gen_cli/CMakeLists.txt")
    add_subdirectory(secret_gen_cli)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cad_builder/CMakeLists.txt")
    add_subdirectory(cad_builder)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/screen_builder/CMakeLists.txt")
    add_subdirectory(screen_builder)
endif()
