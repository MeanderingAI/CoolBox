add_library(self_organizing_maps INTERFACE)
target_include_directories(self_organizing_maps INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(self_organizing_maps INTERFACE mytrix)

# Tests
if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(self_organizing_maps_tests
        tests/test_self_organizing_map.cpp
    )
    target_link_libraries(self_organizing_maps_tests PRIVATE self_organizing_maps tyst_framework_main)
    add_test(NAME SelfOrganizingMapsTests COMMAND self_organizing_maps_tests)
endif()
