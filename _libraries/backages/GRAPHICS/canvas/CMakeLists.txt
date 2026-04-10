add_library(canvas INTERFACE)
target_include_directories(canvas INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/headers>
  $<INSTALL_INTERFACE:include>
)
target_compile_features(canvas INTERFACE cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(canvas_tests tests/test_canvas.cpp)
    target_link_libraries(canvas_tests PRIVATE canvas tyst_framework_main)
    add_test(NAME GraphicsCanvasTests COMMAND canvas_tests)
endif()
