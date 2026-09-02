cmake_minimum_required(VERSION 3.12)

project(xml_screen_descriptor_parser LANGUAGES CXX)

add_library(xml_screen_descriptor_parser STATIC
    source/xml_screen_descriptor_parser.cpp
)

target_include_directories(xml_screen_descriptor_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(xml_screen_descriptor_parser PUBLIC cxx_std_17)
target_link_libraries(xml_screen_descriptor_parser PUBLIC components windows)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(xml_screen_descriptor_parser_tests
        tests/test_xml_screen_descriptor_parser.cpp
    )
    target_link_libraries(xml_screen_descriptor_parser_tests PRIVATE xml_screen_descriptor_parser tyst_framework_main)
    add_test(NAME XmlScreenDescriptorParserTests COMMAND xml_screen_descriptor_parser_tests)
endif()
