
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

# Use a system SQLite3 when available; otherwise build the public-domain amalgamation.
option(COOLBOX_SQL_FETCH_SQLITE "Build bundled SQLite when a system SQLite3 is not found" ON)
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    if(NOT COOLBOX_SQL_FETCH_SQLITE)
        message(FATAL_ERROR "SQLite3 not found. Install it or configure with -DCOOLBOX_SQL_FETCH_SQLITE=ON.")
    endif()
    message(STATUS "SQLite3 not found; building bundled SQLite amalgamation.")
    include(FetchContent)
    FetchContent_Declare(
        coolbox_sqlite_amalgamation
        URL https://www.sqlite.org/2025/sqlite-amalgamation-3500400.zip
        URL_HASH SHA3_256=f131b68e6ba5fb891cc13ebb5ff9555054c77294cb92d8d1268bad5dba4fa2a1
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(coolbox_sqlite_amalgamation)
    add_library(coolbox_sqlite3 STATIC "${coolbox_sqlite_amalgamation_SOURCE_DIR}/sqlite3.c")
    target_include_directories(coolbox_sqlite3 PUBLIC "${coolbox_sqlite_amalgamation_SOURCE_DIR}")
    set_target_properties(coolbox_sqlite3 PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(coolbox_sqlite3 PRIVATE SQLITE_THREADSAFE=1 SQLITE_OMIT_LOAD_EXTENSION)
    if(UNIX)
        find_package(Threads REQUIRED)
        target_link_libraries(coolbox_sqlite3 PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
    endif()
    add_library(SQLite::SQLite3 ALIAS coolbox_sqlite3)
endif()

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
