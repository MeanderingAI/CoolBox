cmake_minimum_required(VERSION 3.15)
project(ToolBoxServices CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Point to the main ToolBox includes and _libraries
set(TOOLBOX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(TOOLBOX_INCLUDE "${TOOLBOX_ROOT}/include")
set(TOOLBOX_LIB_INCLUDE "/Users/mehranghamaty/wkspace/ToolBox/_libraries/include")
set(TOOLBOX_BUILD "${TOOLBOX_ROOT}/build")


include_directories(${TOOLBOX_INCLUDE})
include_directories(${CMAKE_SOURCE_DIR}/../_libraries/include)
add_subdirectory(system_monitor)
add_subdirectory(proxy_service)

# Auto-detect all C++ service files in the current directory
file(GLOB SERVICE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# List to collect all executable targets for install
set(SERVICE_TARGETS "")

# Create an executable for each .cpp file found
foreach(SERVICE_FILE ${SERVICE_SOURCES})
    # Get the filename without path and extension
    get_filename_component(SERVICE_NAME ${SERVICE_FILE} NAME_WE)
    
    if(NOT SERVICE_NAME STREQUAL "metrics_backend_service")
        message(STATUS "Auto-detected service: ${SERVICE_NAME}")
        
        # Create executable
        add_executable(${SERVICE_NAME} ${SERVICE_FILE})
        
        # Default _libraries that most services need
        target_link_libraries(${SERVICE_NAME} PRIVATE pthread)
        target_include_directories(${SERVICE_NAME} PRIVATE ${TOOLBOX_INCLUDE} ${TOOLBOX_LIB_INCLUDE})
        # Ensure linker can find html_processor
        target_link_directories(${SERVICE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/../build/_libraries/src/networking/html)

    # Add service-specific dependencies
    if(${SERVICE_NAME} STREQUAL "metrics_backend_service")
        target_link_libraries(${SERVICE_NAME} PRIVATE system_monitor_lib html_processor)
        target_include_directories(${SERVICE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/system_monitor/include ${TOOLBOX_LIB_INCLUDE})
    elseif(${SERVICE_NAME} STREQUAL "proxy_service")
        target_link_libraries(${SERVICE_NAME} PRIVATE proxy_service_lib)
        target_include_directories(${SERVICE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/proxy_service/include)
        # Proxy service needs OpenSSL for SSL/TLS support
        find_package(OpenSSL)
        if(OPENSSL_FOUND)
            target_link_libraries(${SERVICE_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
            target_compile_definitions(${SERVICE_NAME} PRIVATE HAVE_OPENSSL)
        endif()
    endif()
    
    # Add to install targets list
    list(APPEND SERVICE_TARGETS ${SERVICE_NAME})
    endif()
endforeach()

# Install all auto-detected service targets
if(SERVICE_TARGETS)
    install(TARGETS ${SERVICE_TARGETS}
            RUNTIME DESTINATION bin)
endif()
