# Custom FindSQLite3.cmake
# Usage: find_package(SQLite3 REQUIRED)
# Sets: SQLite3_FOUND, SQLite3_INCLUDE_DIR, SQLite3_LIBRARY, SQLite3_LIBRARIES, SQLite3_VERSION
# Provides imported target: SQLite::SQLite3

# Allow user to specify custom paths
set(SQLite3_ROOT $ENV{SQLITE3_ROOT} CACHE PATH "Root directory of SQLite3 installation")

set(_sqlite3_roots
    ${SQLite3_ROOT}
    $ENV{SQLite3_ROOT}
    $ENV{SQLITE3_ROOT}
)

if (WIN32)
    set(_sqlite3_vcpkg_triplets)
    if(DEFINED VCPKG_TARGET_TRIPLET AND NOT "${VCPKG_TARGET_TRIPLET}" STREQUAL "")
        list(APPEND _sqlite3_vcpkg_triplets "${VCPKG_TARGET_TRIPLET}")
    endif()
    if(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]64$")
        list(APPEND _sqlite3_vcpkg_triplets "arm64-windows")
    elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Xx]64$")
        list(APPEND _sqlite3_vcpkg_triplets "x64-windows")
    endif()
    list(APPEND _sqlite3_vcpkg_triplets "x64-windows")
    list(REMOVE_DUPLICATES _sqlite3_vcpkg_triplets)

    # Windows: look for vcpkg, system, or user-provided
    foreach(_sqlite3_triplet IN LISTS _sqlite3_vcpkg_triplets)
        list(APPEND _sqlite3_roots
            $ENV{VCPKG_ROOT}/installed/${_sqlite3_triplet}
        )
    endforeach()
    list(APPEND _sqlite3_roots
        "$ENV{ProgramFiles}/SQLite"
        "$ENV{ProgramFiles_x86}/SQLite"
        "C:/SQLite"
        "C:/sqlite"
        "C:/msys64/mingw64"
        "C:/msys64/mingw32"
    )
    find_path(SQLite3_INCLUDE_DIR
        NAMES sqlite3.h
        PATHS ${_sqlite3_roots}
        PATH_SUFFIXES include
    )
    find_library(SQLite3_LIBRARY
        NAMES sqlite3 sqlite3.lib
        PATHS ${_sqlite3_roots}
        PATH_SUFFIXES lib
    )
elseif(APPLE)
    # macOS: Homebrew, MacPorts, system
    list(APPEND _sqlite3_roots
        /opt/homebrew
        /opt/homebrew/opt/sqlite
        /usr/local
        /usr/local/opt/sqlite
        /opt/local
        /opt/local/opt/sqlite
        /opt/local/libexec
        /usr
    )
    find_path(SQLite3_INCLUDE_DIR
        NAMES sqlite3.h
        PATHS ${_sqlite3_roots}
        PATH_SUFFIXES include
    )
    find_library(SQLite3_LIBRARY
        NAMES sqlite3
        PATHS ${_sqlite3_roots}
        PATH_SUFFIXES lib
    )
else()
    # Linux/Unix
    list(APPEND _sqlite3_roots
        /usr/local
        /usr
    )
    set(_sqlite3_library_hints)
    if(CMAKE_LIBRARY_ARCHITECTURE)
        list(APPEND _sqlite3_library_hints
            "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}"
            "/lib/${CMAKE_LIBRARY_ARCHITECTURE}"
        )
    endif()
    find_path(SQLite3_INCLUDE_DIR
        NAMES sqlite3.h
        PATHS ${_sqlite3_roots}
        PATH_SUFFIXES include
    )
    find_library(SQLite3_LIBRARY
        NAMES sqlite3
        HINTS ${_sqlite3_library_hints}
        PATHS ${_sqlite3_roots}
        PATH_SUFFIXES lib
    )
endif()

if(NOT SQLite3_INCLUDE_DIR)
    find_path(SQLite3_INCLUDE_DIR NAMES sqlite3.h)
endif()

if(NOT SQLite3_LIBRARY)
    find_library(SQLite3_LIBRARY
        NAMES sqlite3 sqlite3.lib
        HINTS ${_sqlite3_library_hints}
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SQLite3
    REQUIRED_VARS SQLite3_LIBRARY SQLite3_INCLUDE_DIR
    VERSION_VAR SQLite3_VERSION
)

if(NOT SQLite3_FOUND)
    message("")
    message(WARNING "SQLite3 development files not found!")
    message("")
    message(STATUS "To build this project, you must install SQLite3 development files.")
    message(STATUS "On Windows: Use vcpkg (https://github.com/microsoft/vcpkg): vcpkg install sqlite3")
    message(STATUS "  Or download prebuilt binaries from https://www.sqlite.org/download.html and set the SQLITE3_ROOT environment variable.")
    message(STATUS "On macOS: brew install sqlite3")
    message(STATUS "On Linux: sudo apt-get install libsqlite3-dev")
    message("")
    message(FATAL_ERROR "SQLite3 not found. See instructions above.")
endif()

if(SQLite3_FOUND)
    set(SQLite3_LIBRARIES ${SQLite3_LIBRARY})
    set(SQLite3_INCLUDE_DIRS ${SQLite3_INCLUDE_DIR})
    if(NOT TARGET SQLite::SQLite3)
        add_library(SQLite::SQLite3 UNKNOWN IMPORTED)
        set_target_properties(SQLite::SQLite3 PROPERTIES
            IMPORTED_LOCATION "${SQLite3_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${SQLite3_INCLUDE_DIR}"
        )
    endif()
endif()
