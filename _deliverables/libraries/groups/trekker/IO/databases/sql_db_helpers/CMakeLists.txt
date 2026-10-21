
add_library(sql SHARED
    source/schema_parser.cpp
    source/query_builder.cpp
    source/database.cpp
)

option(COOLBOX_SQL_ENABLE_POSTGRESQL "Build the PostgreSQL database backend" OFF)
option(COOLBOX_SQL_ENABLE_MYSQL "Build the MySQL database backend" OFF)
get_filename_component(COOLBOX_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../" ABSOLUTE)

target_include_directories(sql PUBLIC
    ${COOLBOX_ROOT_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${COOLBOX_ROOT_DIR}/_deliverables/libraries/groups/trekker/IO/dataformats/json/headers
)

# Find SQLite3
find_package(SQLite3 REQUIRED)

target_link_libraries(sql
    json
    SQLite::SQLite3
)

if(COOLBOX_SQL_ENABLE_POSTGRESQL)
    find_package(PostgreSQL REQUIRED)
    target_sources(sql PRIVATE source/postgresql_database.cpp)
    target_link_libraries(sql PRIVATE PostgreSQL::PostgreSQL)
    target_compile_definitions(sql PRIVATE COOLBOX_SQL_ENABLE_POSTGRESQL)
endif()

if(COOLBOX_SQL_ENABLE_MYSQL)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(MYSQLCLIENT QUIET IMPORTED_TARGET libmariadb)
        if(NOT TARGET PkgConfig::MYSQLCLIENT)
            pkg_check_modules(MYSQLCLIENT QUIET IMPORTED_TARGET mysqlclient)
        endif()
    endif()

    if(TARGET PkgConfig::MYSQLCLIENT)
        target_link_libraries(sql PRIVATE PkgConfig::MYSQLCLIENT)
    else()
        find_path(MYSQLCLIENT_INCLUDE_DIR NAMES mysql.h PATH_SUFFIXES mysql mariadb)
        find_library(MYSQLCLIENT_LIBRARY NAMES mariadb mysqlclient)
        if(NOT MYSQLCLIENT_INCLUDE_DIR OR NOT MYSQLCLIENT_LIBRARY)
            message(FATAL_ERROR "COOLBOX_SQL_ENABLE_MYSQL requires MySQL Connector/C or MariaDB Connector/C")
        endif()
        target_include_directories(sql PRIVATE "${MYSQLCLIENT_INCLUDE_DIR}")
        target_link_libraries(sql PRIVATE "${MYSQLCLIENT_LIBRARY}")
    endif()

    target_sources(sql PRIVATE source/mysql_database.cpp)
    target_compile_definitions(sql PRIVATE COOLBOX_SQL_ENABLE_MYSQL)
endif()

if(BUILD_TESTING)
    add_executable(test_sql_dialects tests/test_sql_dialects.cpp)
    target_link_libraries(test_sql_dialects PRIVATE sql)
    add_test(NAME test_sql_dialects COMMAND test_sql_dialects)
endif()
