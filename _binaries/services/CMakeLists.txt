# Automatically add all subdirectories with CMakeLists.txt
file(GLOB SERVICE_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} service_*)
foreach(dir ${SERVICE_DIRS})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt")
            add_subdirectory(${dir})
        endif()
    endif()
endforeach()
