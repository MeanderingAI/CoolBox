cmake_minimum_required(VERSION 4.3.1)
project(PARSER_MATLAB LANGUAGES CXX)

include(GNUInstallDirs)
if(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
  set(CMAKE_INSTALL_INCLUDEDIR "include")
endif()

add_library(matlab_parser STATIC
  src/parser.cpp
)
target_include_directories(matlab_parser PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

install(TARGETS matlab_parser)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  enable_testing()
  add_executable(test_matlab_parser tests/test_matlab_parser.cpp)
  target_link_libraries(test_matlab_parser PRIVATE matlab_parser)
  add_test(NAME parser::matlab COMMAND test_matlab_parser)
endif()
