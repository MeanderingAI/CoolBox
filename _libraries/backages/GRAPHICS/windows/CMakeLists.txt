
cmake_minimum_required(VERSION 4.3.1)
project(windows)

add_library(windows
    source/windows.cpp
)

target_include_directories(windows PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(windows PUBLIC cxx_std_17)
target_link_libraries(windows PUBLIC charts components json)

if(BUILD_TESTING AND TARGET GTest::gtest_main)
    enable_testing()
    add_executable(windows_tests tests/test_windows.cpp)
    target_link_libraries(windows_tests PRIVATE windows GTest::gtest_main)
    if(WIN32)
        add_custom_command(TARGET windows_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:components>
                $<TARGET_FILE:json>
                $<TARGET_FILE_DIR:windows_tests>
        )
    endif()
    add_test(NAME WindowsSimulationTests COMMAND windows_tests)
endif()
