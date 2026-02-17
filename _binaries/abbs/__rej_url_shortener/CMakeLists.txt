add_executable(url_shortener
    url_shortener.cpp
)

target_include_directories(url_shortener PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(url_shortener PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "url_shortener_bin"
)
