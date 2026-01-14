add_library(sql SHARED
    schema_parser.cpp
    query_builder.cpp
    database.cpp
)

target_include_directories(sql PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Find SQLite3
find_package(SQLite3 REQUIRED)

target_link_libraries(sql
    json
    SQLite::SQLite3
)
