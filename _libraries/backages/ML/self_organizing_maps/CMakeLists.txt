add_library(self_organizing_maps INTERFACE)
target_include_directories(self_organizing_maps INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Link Eigen (fetched at top level)
if(TARGET Eigen3::Eigen)
    target_link_libraries(self_organizing_maps INTERFACE Eigen3::Eigen)
elseif(TARGET eigen)
    target_link_libraries(self_organizing_maps INTERFACE eigen)
endif()

# Tests
enable_testing()
add_executable(self_organizing_maps_tests
    tests/test_self_organizing_map.cpp
)
target_link_libraries(self_organizing_maps_tests PRIVATE self_organizing_maps GTest::gtest_main)
add_test(NAME SelfOrganizingMapsTests COMMAND self_organizing_maps_tests)
