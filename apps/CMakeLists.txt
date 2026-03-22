cmake_minimum_required(VERSION 3.12)

# Add individual apps if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp/CMakeLists.txt")
    add_subdirectory(lsp)
endif()
