# Use FetchContent to manage external dependencies
include(FetchContent)

# Fetch and configure Eigen
FetchContent_Declare(
  Eigen
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG 3.4.0
  SOURCE_DIR ${CMAKE_BINARY_DIR}/eigen-src
  BINARY_DIR ${CMAKE_BINARY_DIR}/eigen-build
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(Eigen)

# Fetch and configure Googletest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  SOURCE_DIR ${CMAKE_BINARY_DIR}/googletest-src
  BINARY_DIR ${CMAKE_BINARY_DIR}/googletest-build
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

# On Windows/vcpkg, prefer CONFIG mode to avoid CMake's FindGSL triggering
# a Fortran compiler search (via FindBLAS).  Fall back to MODULE mode for
# Linux/macOS where the system-installed GSL ships a .pc / FindGSL works fine.
if(WIN32)
  find_package(GSL CONFIG REQUIRED)
else()
  find_package(GSL REQUIRED)
endif()