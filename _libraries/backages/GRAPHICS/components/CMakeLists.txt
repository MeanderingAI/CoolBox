add_library(components SHARED
    source/components.cpp
)

target_include_directories(components PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(components PUBLIC cxx_std_17)

if(BUILD_TESTING AND TARGET GTest::gtest_main)
    enable_testing()
    add_executable(components_tests tests/test_components.cpp)
    target_link_libraries(components_tests PRIVATE components GTest::gtest_main)
    if(WIN32)
        add_custom_command(TARGET components_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:components>
                $<TARGET_FILE_DIR:components_tests>
        )
    endif()
    add_test(NAME GraphicsComponentsTests COMMAND components_tests)
endif()
