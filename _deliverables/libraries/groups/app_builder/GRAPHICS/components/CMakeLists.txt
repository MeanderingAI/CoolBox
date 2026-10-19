add_library(components SHARED
    source/components.cpp
    source/color_picker.cpp
)

target_include_directories(components PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(components PUBLIC cxx_std_17)
target_link_libraries(components PUBLIC charts)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(components_tests tests/test_components.cpp tests/test_color_picker.cpp)
    target_link_libraries(components_tests PRIVATE components tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(components_tests)
    add_test(NAME GraphicsComponentsTests COMMAND components_tests)
endif()
