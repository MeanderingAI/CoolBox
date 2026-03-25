cmake_minimum_required(VERSION 3.12)
project(application_context LANGUAGES CXX)

add_library(application_context SHARED
    source/application_context.cpp
)

target_include_directories(application_context PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

target_compile_features(application_context PUBLIC cxx_std_17)

# If the GRAPHICS backage directory exists in the repo, add it so targets like
# `windows` and `components` are available when building this subpackage standalone.
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
if(EXISTS "${REPO_ROOT}/_libraries/backages/GRAPHICS/CMakeLists.txt")
    add_subdirectory(${REPO_ROOT}/_libraries/backages/GRAPHICS ${CMAKE_BINARY_DIR}/graphics_build)
    # Also add key GRAPHICS subpackages so targets like `windows` and `components` exist
    if(EXISTS "${REPO_ROOT}/_libraries/backages/GRAPHICS/components/CMakeLists.txt")
        add_subdirectory(${REPO_ROOT}/_libraries/backages/GRAPHICS/components ${CMAKE_BINARY_DIR}/graphics_components_build)
    endif()
    if(EXISTS "${REPO_ROOT}/_libraries/backages/GRAPHICS/charts/CMakeLists.txt")
        add_subdirectory(${REPO_ROOT}/_libraries/backages/GRAPHICS/charts ${CMAKE_BINARY_DIR}/graphics_charts_build)
    endif()
    if(EXISTS "${REPO_ROOT}/_libraries/backages/IO/dataformats/json/CMakeLists.txt")
        add_subdirectory(${REPO_ROOT}/_libraries/backages/IO/dataformats/json ${CMAKE_BINARY_DIR}/json_build)
    endif()
    if(EXISTS "${REPO_ROOT}/_libraries/backages/GRAPHICS/windows/CMakeLists.txt")
        add_subdirectory(${REPO_ROOT}/_libraries/backages/GRAPHICS/windows ${CMAKE_BINARY_DIR}/graphics_windows_build)
    endif()
endif()

target_link_libraries(application_context PUBLIC
        windows
        components
)

install(TARGETS application_context
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
