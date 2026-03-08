
add_library(meaning_analysis SHARED
    source/meaning_analysis.cpp
)

target_include_directories(meaning_analysis PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../../syntax_analysis/ast/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lexical_analysis/parser/headers
)

target_link_libraries(meaning_analysis PUBLIC
    sql_ast
    sql_parser
)

set_target_properties(meaning_analysis PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
