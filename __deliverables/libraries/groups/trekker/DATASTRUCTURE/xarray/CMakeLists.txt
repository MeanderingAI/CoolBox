add_library(xarray INTERFACE)
target_include_directories(xarray INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/headers)

enable_testing()
add_executable(xarray_tests tests/test_xarray.cpp)
target_link_libraries(xarray_tests PRIVATE xarray)
add_test(NAME XArrayTests COMMAND xarray_tests)
