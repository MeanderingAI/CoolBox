
# Add subdirectories if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/http/CMakeLists.txt")
	add_subdirectory(http)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/json" AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/json")
	add_subdirectory(json)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/web/CMakeLists.txt")
	add_subdirectory(web)
endif()