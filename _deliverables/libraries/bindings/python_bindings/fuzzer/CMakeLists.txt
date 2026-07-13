if(NOT BUILD_PYTHON_BINDINGS)
	message(STATUS "Skipping fuzzer_bindings (BUILD_PYTHON_BINDINGS=OFF)")
	return()
endif()

if(NOT TARGET pybind11::pybind11)
	message(STATUS "Skipping fuzzer_bindings (pybind11 target not available)")
	return()
endif()

pybind11_add_module(fuzzer_bindings fuzzer_bindings.cpp)
target_include_directories(fuzzer_bindings PRIVATE
	${CMAKE_SOURCE_DIR}/_deliverables/libraries/include
	${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/sim_group/SECURITY/fuzzer/headers
)
