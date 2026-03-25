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

if(BUILD_TESTING)
  # Prefer an installed/system GTest if available, otherwise fetch via FetchContent
  find_package(GTest QUIET)
  if(NOT TARGET GTest::gtest_main)
    message(STATUS "GTest not found by find_package; fetching googletest via FetchContent...")
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
      SOURCE_DIR ${CMAKE_BINARY_DIR}/googletest-src
      BINARY_DIR ${CMAKE_BINARY_DIR}/googletest-build
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)

    # Ensure the expected imported targets exist in the GTest:: namespace
    if(TARGET gtest_main AND NOT TARGET GTest::gtest_main)
      add_library(GTest::gtest_main ALIAS gtest_main)
    endif()
    if(TARGET gtest AND NOT TARGET GTest::gtest)
      add_library(GTest::gtest ALIAS gtest)
    endif()
  else()
    message(STATUS "Using system-provided GTest targets")
  endif()
else()
  message(STATUS "BUILD_TESTING is OFF; skipping GTest fetch")
endif()


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

# Make GSL optional; FindGSL (MODULE mode) may trigger FindBLAS which
# probes for a Fortran compiler. Disabling GSL on Windows avoids failing
# configuration when GSL isn't installed on developer machines.
option(ENABLE_GSL "Enable GSL (GNU Scientific Library) support" ON)
# Allow CI/toolchains to enable GSL on Windows by providing GSL paths via
# environment variables (GSL_INCLUDE_DIR/GSL_LIB_DIR) or vcpkg. Do not
# forcibly disable on WIN32; instead respect the option or CI-provided vars.
if(DEFINED ENV{GSL_LIB_DIR} OR DEFINED ENV{GSL_INCLUDE_DIR})
  set(ENABLE_GSL ON CACHE BOOL "Enable GSL (GNU Scientific Library) support" FORCE)
endif()

if(ENABLE_GSL)
  # FindGSL may probe for BLAS/Fortran; ensure toolchain provides them if enabled.
  # Allow cached variables from CI (GSL_INCLUDE_DIR/GSL_LIBRARY/GSL_CBLAS_LIBRARY) to help FindGSL.
  find_package(GSL REQUIRED QUIET)
else()
  message(STATUS "GSL support is disabled (ENABLE_GSL=OFF). To enable, install GSL and reconfigure with -DENABLE_GSL=ON")
endif()