# This CMake script detects installed Visual Studio generators and prints them.
# Usage: cmake -P _scripts/detect_vs_generator.cmake

message(STATUS "Detecting available Visual Studio generators...")

set(_generators)
execute_process(
  COMMAND cmake --help
  OUTPUT_VARIABLE _cmake_help
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(REGEX MATCHALL "Visual Studio [0-9]+ [0-9]+" _generators "${_cmake_help}")

if(_generators)
  message(STATUS "Found Visual Studio generators:")
  foreach(gen IN LISTS _generators)
    message(STATUS "  ${gen}")
  endforeach()
else()
  message(WARNING "No Visual Studio generators found in cmake --help output.")
endif()
