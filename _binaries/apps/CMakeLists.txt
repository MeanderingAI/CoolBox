# Automatically add all subdirectories with CMakeLists.txt
file(GLOB APP_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} app_*)
foreach(dir ${APP_DIRS})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt")
            add_subdirectory(${dir})
        endif()
    endif()
endforeach()