pybind11_add_module(fuzzer_bindings fuzzer_bindings.cpp)
target_include_directories(fuzzer_bindings PRIVATE
	${CMAKE_SOURCE_DIR}/_deliverables/libraries/include
	${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/sim_group/SECURITY/fuzzer/headers
)
