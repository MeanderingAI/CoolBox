
# SQL databases — frontend (parsing/validation) + db helpers

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/frontend" AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/frontend")
	add_subdirectory(frontend)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sql_db_helpers" AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sql_db_helpers")
	add_subdirectory(sql_db_helpers)
endif()
