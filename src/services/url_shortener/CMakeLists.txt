add_library(url_shortener
    url_shortener.cpp
)

target_include_directories(url_shortener PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
