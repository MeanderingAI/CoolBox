include(${CMAKE_SOURCE_DIR}/cmake/ExternalDependencies.cmake)
pybind11_add_module(http_bindings http_bindings.cpp)
target_include_directories(http_bindings PRIVATE
	${CMAKE_SOURCE_DIR}/_libraries/include
	${CMAKE_SOURCE_DIR}/_libraries/groups/trekker/IO
	${CMAKE_SOURCE_DIR}/_libraries/groups/trekker/IO/dataformats/json/headers
)
