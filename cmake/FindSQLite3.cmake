# Custom FindSQLite3.cmake
# Usage: find_package(SQLite3 REQUIRED)
# Sets: SQLite3_FOUND, SQLite3_INCLUDE_DIR, SQLite3_LIBRARY, SQLite3_LIBRARIES, SQLite3_VERSION
# Provides imported target: SQLite::SQLite3

# Allow user to specify custom paths
set(SQLite3_ROOT $ENV{SQLITE3_ROOT} CACHE PATH "Root directory of SQLite3 installation")

if (WIN32)
    # Windows: look for vcpkg, system, or user-provided
    find_path(SQLite3_INCLUDE_DIR
        NAMES sqlite3.h
        PATHS
            ${SQLite3_ROOT}/include
            $ENV{VCPKG_ROOT}/installed/x64-windows/include
            "$ENV{ProgramFiles}/SQLite/include"
            "$ENV{ProgramFiles_x86}/SQLite/include" # CMake does not allow () in ENV var names, use ProgramFiles_x86
            "C:/SQLite/include"
            "C:/sqlite/include"
            "C:/msys64/mingw64/include"
            "C:/msys64/mingw32/include"
        PATH_SUFFIXES include
        NO_DEFAULT_PATH
    )
    find_library(SQLite3_LIBRARY
        NAMES sqlite3 sqlite3.lib
        PATHS
            ${SQLite3_ROOT}/lib
            $ENV{VCPKG_ROOT}/installed/x64-windows/lib
            "$ENV{ProgramFiles}/SQLite/lib"
            "$ENV{ProgramFiles_x86}/SQLite/lib" # CMake does not allow () in ENV var names, use ProgramFiles_x86
            "C:/SQLite/lib"
            "C:/sqlite/lib"
            "C:/msys64/mingw64/lib"
            "C:/msys64/mingw32/lib"
        PATH_SUFFIXES lib
        NO_DEFAULT_PATH
    )
elseif(APPLE)
    # macOS: Homebrew, MacPorts, system
    find_path(SQLite3_INCLUDE_DIR
        NAMES sqlite3.h
        PATHS
            ${SQLite3_ROOT}/include
            /usr/local/include
            /opt/homebrew/include
            /opt/local/include
            /usr/include
        PATH_SUFFIXES include
        NO_DEFAULT_PATH
    )
    find_library(SQLite3_LIBRARY
        NAMES sqlite3
        PATHS
            ${SQLite3_ROOT}/lib
            /usr/local/lib
            /opt/homebrew/lib
            /opt/local/lib
            /usr/lib
        PATH_SUFFIXES lib
        NO_DEFAULT_PATH
    )
else()
    # Linux/Unix
    find_path(SQLite3_INCLUDE_DIR
        NAMES sqlite3.h
        PATHS
            ${SQLite3_ROOT}/include
            /usr/local/include
            /usr/include
            /usr/include/sqlite3
        PATH_SUFFIXES include
        NO_DEFAULT_PATH
    )
    find_library(SQLite3_LIBRARY
        NAMES sqlite3
        PATHS
            ${SQLite3_ROOT}/lib
            /usr/local/lib
            /usr/lib
            /usr/lib/x86_64-linux-gnu
        PATH_SUFFIXES lib
        NO_DEFAULT_PATH
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
