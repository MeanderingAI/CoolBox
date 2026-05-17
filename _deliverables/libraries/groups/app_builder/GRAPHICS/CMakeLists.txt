#this isn't used
cmake_minimum_required(VERSION 3.12)
project(GRAPHICS LANGUAGES C CXX)

# Top-level convenience target for consumers
add_library(graphics INTERFACE)
target_include_directories(graphics INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/components/headers>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/fonts/headers>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/canvas/headers>
  $<INSTALL_INTERFACE:include>
)

# Install headers from subpackages
install(DIRECTORY components/headers/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/graphics)
install(DIRECTORY fonts/headers/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/graphics)
install(DIRECTORY canvas/headers/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/graphics)
install(TARGETS graphics)

# Optional texture loader library (uses stb_image if available in third_party)
option(GRAPHICS_ENABLE_STB_IMAGE "Enable stb_image-based texture loader if stb_image.h is available in third_party" ON)

set(STB_PATH "${CMAKE_SOURCE_DIR}/third_party/stb_image.h")
if(GRAPHICS_ENABLE_STB_IMAGE AND EXISTS "${STB_PATH}")
  add_library(graphics_textures STATIC canvas/source/texture_loader_stb.cpp)
  target_include_directories(graphics_textures PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/canvas/headers>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party>
    $<INSTALL_INTERFACE:include>
  )
  target_compile_definitions(graphics_textures PUBLIC HAVE_STB_IMAGE)
  install(TARGETS graphics_textures
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
else()
  add_library(graphics_textures INTERFACE)
  target_include_directories(graphics_textures INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/canvas/headers>
    $<INSTALL_INTERFACE:include>
  )
  install(TARGETS graphics_textures)
endif()

# Propagate textures to the top-level convenience target
if(TARGET graphics_textures)
  target_link_libraries(graphics INTERFACE graphics_textures)
endif()

# The local test runtime is provided by the TOOLS/tyst_framework package.
# Subpackages should link `tyst_framework_main` if the target exists.
if(BUILD_TESTING)
  if(NOT TARGET tyst_framework_main)
    message(STATUS "tyst_framework_main not available; GRAPHICS tests will be skipped")
  endif()
endif()

# Add subdirectories (keep the layout explicit and consistent)
add_subdirectory(components)
add_subdirectory(charts)
add_subdirectory(fonts)
add_subdirectory(canvas)
