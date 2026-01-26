
# Generate all static asset headers (html, js, css)
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/_static_assets/resources")
set(ASSETS_STAMP "${CMAKE_SOURCE_DIR}/_binaries/apps/app_service_manager/include/static_assets/all_assets_generated.stamp")
add_custom_command(
    OUTPUT ${ASSETS_STAMP}
    COMMAND python3 ${ASSETS_DIR}/html_to_cpp.py ${ASSETS_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${ASSETS_STAMP}
    DEPENDS ${ASSETS_DIR}/html_to_cpp.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating all static asset headers"
)
add_custom_target(static_assets_headers ALL DEPENDS ${ASSETS_STAMP})

add_executable(service_manager service_manager.cpp src/request_handlers.cpp src/utils.cpp)
add_dependencies(service_manager static_assets_headers)
target_include_directories(service_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/_libraries/include
    ${CMAKE_SOURCE_DIR}/_binaries/apps/app_service_manager/include
    ${CMAKE_SOURCE_DIR}/_binaries/apps/app_service_manager/src
    /Users/mehranghamaty/wkspace/ToolTool/_libraries/packages/IO/dataformats/http/headers
    /Users/mehranghamaty/wkspace/ToolTool/_libraries/packages/IO/http_server
    /Users/mehranghamaty/wkspace/ToolTool/_libraries/packages/IO/thread_management/headers
    /Users/mehranghamaty/wkspace/ToolTool/_libraries/packages/utils/unix_commands
    /Users/mehranghamaty/wkspace/ToolTool/_libraries/packages
)
target_link_libraries(service_manager
    json
    advanced_logging
    pthread
    http_server
    unix_commands
)
set_target_properties(service_manager PROPERTIES CXX_STANDARD 17)
