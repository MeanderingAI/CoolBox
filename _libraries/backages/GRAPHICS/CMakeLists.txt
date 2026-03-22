cmake_minimum_required(VERSION 3.12)
project(GRAPHICS_CUSTOM LANGUAGES C CXX)

add_library(graphics_custom INTERFACE)
target_include_directories(graphics_custom INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Install headers from subpackages (we moved headers into fonts/headers and components/headers)
install(DIRECTORY components/headers/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/graphics)
install(DIRECTORY fonts/headers/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/graphics)
install(TARGETS graphics_custom)

# Optional texture loader library (uses stb_image if available in third_party)
option(GRAPHICS_ENABLE_STB_IMAGE "Enable stb_image-based texture loader if stb_image.h is available in third_party" ON)

set(STB_PATH "${CMAKE_SOURCE_DIR}/third_party/stb_image.h")
if(GRAPHICS_ENABLE_STB_IMAGE AND EXISTS "${STB_PATH}")
  add_library(graphics_textures STATIC components/source/texture_loader_stb.cpp)
  target_include_directories(graphics_textures PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/components/headers>
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
  # Provide a stub target so consumers can link unconditionally if desired
  add_library(graphics_textures INTERFACE)
  target_include_directories(graphics_textures INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  )
  install(TARGETS graphics_textures)
endif()

# Make the interface convenience target propagate dependency
if(TARGET graphics_textures)
  target_link_libraries(graphics_custom INTERFACE graphics_textures)
endif()
