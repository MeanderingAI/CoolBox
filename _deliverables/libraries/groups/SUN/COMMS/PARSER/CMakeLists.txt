add_library(sun_parser
    source/sun_command_parser.cpp
)

target_include_directories(sun_parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

target_compile_features(sun_parser PUBLIC cxx_std_17)

target_link_libraries(sun_parser PUBLIC sun_misc)
