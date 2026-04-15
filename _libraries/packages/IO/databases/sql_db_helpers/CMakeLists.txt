
add_library(sql SHARED
    source/schema_parser.cpp
    source/query_builder.cpp
    source/database.cpp
)

target_include_directories(sql PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_SOURCE_DIR}/_libraries/packages/IO/dataformats/json/headers
)

# Find SQLite3
find_package(SQLite3 REQUIRED)

target_link_libraries(sql
    json
    SQLite::SQLite3
)
