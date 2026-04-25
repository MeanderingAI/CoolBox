if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/MStudio/CMakeLists.txt")
    add_subdirectory(MStudio)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/body_generator/CMakeLists.txt")
    add_subdirectory(body_generator)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bower_shell/CMakeLists.txt")
    add_subdirectory(bower_shell)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/file_browser/CMakeLists.txt")
    add_subdirectory(file_browser)
endif()