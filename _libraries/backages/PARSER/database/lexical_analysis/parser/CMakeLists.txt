
add_library(sql_parser SHARED
    source/sql_tokenizer.cpp
)

target_include_directories(sql_parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

set_target_properties(sql_parser PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
