cmake_minimum_required(VERSION 3.15)
project(ToolBoxServices CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Point to the main ToolBox includes and libraries
set(TOOLBOX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(TOOLBOX_INCLUDE "${TOOLBOX_ROOT}/include")
set(TOOLBOX_BUILD "${TOOLBOX_ROOT}/build")

include_directories(${TOOLBOX_INCLUDE})
link_directories(${TOOLBOX_BUILD}/src/services/system_monitor)

# Metrics Backend Service
add_executable(metrics_backend_service metrics_backend_service.cpp)
target_link_libraries(metrics_backend_service PRIVATE system_monitor pthread)
target_include_directories(metrics_backend_service PRIVATE ${TOOLBOX_INCLUDE})

# Account Service
add_executable(account_service account_service.cpp)
target_link_libraries(account_service PRIVATE pthread)
target_include_directories(account_service PRIVATE ${TOOLBOX_INCLUDE})

# Install targets
install(TARGETS metrics_backend_service account_service
        RUNTIME DESTINATION bin)
