cmake_minimum_required(VERSION 3.12)
project(CHEMISTRY LANGUAGES C CXX)

include(GNUInstallDirs)
if(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
  set(CMAKE_INSTALL_INCLUDEDIR "include")
endif()

add_library(chemistry INTERFACE)
target_include_directories(chemistry INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS chemistry)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  enable_testing()
  add_executable(test_chemistry
    tests/test_periodic_table.cpp
  )
  target_include_directories(test_chemistry PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(test_chemistry PRIVATE chemistry)
  add_test(NAME chemistry::periodic_table COMMAND test_chemistry)
endif()
