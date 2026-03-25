
add_library(grammar_validation SHARED
    source/grammar_validation.cpp
)

target_include_directories(grammar_validation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../ast/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lexical_analysis/parser/headers
)

target_link_libraries(grammar_validation PUBLIC
    sql_ast
    sql_parser
)

set_target_properties(grammar_validation PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
