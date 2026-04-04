set(FONTS_LIBRARY_TYPE SHARED)
if(WIN32)
    set(FONTS_LIBRARY_TYPE STATIC)
endif()

add_library(fonts ${FONTS_LIBRARY_TYPE}
    source/fonts.cpp
    source/stb_truetype.cpp
)

target_include_directories(fonts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(fonts PUBLIC cxx_std_17)
target_link_libraries(fonts PUBLIC charts)

if(BUILD_TESTING AND TARGET GTest::gtest_main)
    enable_testing()
    add_executable(fonts_tests tests/test_fonts.cpp)
    target_link_libraries(fonts_tests PRIVATE fonts GTest::gtest_main)
    add_test(NAME GraphicsFontsTests COMMAND fonts_tests)
endif()
