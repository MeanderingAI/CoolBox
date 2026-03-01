# Use FetchContent to manage external dependencies
include(FetchContent)

# Fetch and configure Eigen
# Disable Eigen's own tests/docs to avoid DetermineOSVersion warnings and
# unnecessary Fortran compiler probes from EigenTesting.cmake
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(EIGEN_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(EIGEN_BUILD_PKGCONFIG OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  Eigen
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG 3.4.0
  SOURCE_DIR ${CMAKE_BINARY_DIR}/eigen-src
  BINARY_DIR ${CMAKE_BINARY_DIR}/eigen-build
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(Eigen)
# Re-enable BUILD_TESTING for our own project tests
set(BUILD_TESTING ON CACHE BOOL "" FORCE)

# Fetch and configure Googletest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  SOURCE_DIR ${CMAKE_BINARY_DIR}/googletest-src
  BINARY_DIR ${CMAKE_BINARY_DIR}/googletest-build
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)


# Find or fetch Doxygen

find_package(Doxygen QUIET)
if(NOT DOXYGEN_FOUND)
  message(STATUS "Doxygen not found, fetching via FetchContent...")
  include(FetchContent)
endif()

# =============================
# Fetch quiche (QUIC/HTTP3)
# =============================
# quiche is a Rust/Cargo project, not CMake. We fetch it for its C headers
# but do NOT call FetchContent_MakeAvailable (which would try to add_subdirectory).
include(FetchContent)
FetchContent_Declare(
  quiche
  GIT_REPOSITORY https://github.com/cloudflare/quiche.git
  GIT_TAG 0.21.0
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/quiche
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# Use FetchContent_Populate directly (quiche is Rust/Cargo, not CMake)
# Suppress CMP0169 deprecation warning
cmake_policy(SET CMP0169 OLD)
FetchContent_Populate(quiche)
cmake_policy(SET CMP0169 NEW)

# FindGSL (MODULE mode) may trigger FindBLAS which probes for a Fortran
# compiler.  Ensure gfortran is on the PATH in CI (see release.yml).
find_package(GSL REQUIRED)