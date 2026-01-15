add_executable(service_manager service_manager.cpp src/request_handlers.cpp src/utils.cpp)
target_include_directories(service_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/_libraries/include
    ${CMAKE_SOURCE_DIR}/_binaries/apps/app_service_manager/include
    ${CMAKE_SOURCE_DIR}/_binaries/apps/app_service_manager/src
)
target_link_libraries(service_manager
    json
    advanced_logging
    pthread
)
set_target_properties(service_manager PROPERTIES CXX_STANDARD 17)
