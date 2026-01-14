# Automatically add all .cpp files as executables
file(GLOB EXAMPLE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
foreach(example ${EXAMPLE_FILES})
	get_filename_component(example_name ${example} NAME_WE)
	add_executable(${example_name} ${example})
endforeach()
# CMakeLists.txt for _binaries/examples
# Add your example executables here

# Example:
# add_executable(example_name cpp/example_source.cpp)
