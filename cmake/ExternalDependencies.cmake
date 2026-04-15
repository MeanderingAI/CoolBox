# Use FetchContent to manage external dependencies
include(FetchContent)

# Fetch and configure Eigen
# Disable Eigen's own tests/docs to avoid DetermineOSVersion warnings and
# unnecessary Fortran compiler probes from EigenTesting.cmake
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(EIGEN_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(EIGEN_BUILD_PKGCONFIG OFF CACHE BOOL "" FORCE)
set(EIGEN_TEST_NOQT ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  Eigen
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG 3.4.0
  SOURCE_DIR ${CMAKE_BINARY_DIR}/eigen-src
  BINARY_DIR ${CMAKE_BINARY_DIR}/eigen-build
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(_coolbox_old_warn_deprecated ${CMAKE_WARN_DEPRECATED})
set(CMAKE_WARN_DEPRECATED OFF)
FetchContent_MakeAvailable(Eigen)
set(CMAKE_WARN_DEPRECATED ${_coolbox_old_warn_deprecated})
# Re-enable BUILD_TESTING for our own project tests
set(BUILD_TESTING ON CACHE BOOL "" FORCE)


# Find or fetch Doxygen

find_package(Doxygen QUIET)
if(NOT DOXYGEN_FOUND)
  message(STATUS "Doxygen not found, fetching via FetchContent...")
  include(FetchContent)
endif()

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