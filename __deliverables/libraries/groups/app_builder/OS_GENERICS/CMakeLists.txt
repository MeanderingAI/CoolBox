cmake_minimum_required(VERSION 3.16)
get_filename_component(PACKAGE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PACKAGE_NAME} NONE)
message(STATUS "Configuring package: ${PACKAGE_NAME}")

file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*")

# Add windows first when present because full_application_window links against it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/windows" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/windows/CMakeLists.txt")
  message(STATUS "  adding subdir windows")
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/windows" "${CMAKE_BINARY_DIR}/${PACKAGE_NAME}_windows_build")
endif()

foreach(child ${children})
  if(child STREQUAL "windows")
    continue()
  endif()
  if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${child}" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${child}/CMakeLists.txt")
    message(STATUS "  adding subdir ${child}")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/${child}" "${CMAKE_BINARY_DIR}/${PACKAGE_NAME}_${child}_build")
  endif()
endforeach()