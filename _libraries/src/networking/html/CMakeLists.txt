# HTML processing library
add_library(html_processor SHARED
    html_processor.cpp
    web_components.cpp
)

target_include_directories(html_processor PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(html_processor PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
