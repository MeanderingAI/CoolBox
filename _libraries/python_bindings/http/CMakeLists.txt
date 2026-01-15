pybind11_add_module(http_bindings http_bindings.cpp)
target_include_directories(http_bindings PRIVATE ${CMAKE_SOURCE_DIR}/_libraries/include)
