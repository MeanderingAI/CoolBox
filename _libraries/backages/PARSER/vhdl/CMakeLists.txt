cmake_minimum_required(VERSION 3.12)
project(PARSER_VHDL LANGUAGES CXX)

include(GNUInstallDirs)
if(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
  set(CMAKE_INSTALL_INCLUDEDIR "include")
endif()

add_library(vhdl_parser STATIC
  src/parser.cpp
)
target_include_directories(vhdl_parser PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

install(TARGETS vhdl_parser)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  enable_testing()
  add_executable(test_vhdl_parser tests/test_vhdl_parser.cpp)
  target_link_libraries(test_vhdl_parser PRIVATE vhdl_parser)
  add_test(NAME parser::vhdl COMMAND test_vhdl_parser)
endif()
