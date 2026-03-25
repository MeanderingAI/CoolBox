
add_library(sql_ast SHARED
    source/sql_ast.cpp
)

target_include_directories(sql_ast PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lexical_analysis/parser/headers
)

target_link_libraries(sql_ast PUBLIC
    sql_parser
)

set_target_properties(sql_ast PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
