# --- pybind11 (for Python bindings) ---
include(FetchContent)
set(_coolbox_skip_pybind11 FALSE)
set(_coolbox_skip_pybind11_reason "")

if(DEFINED BUILD_PYTHON_BINDINGS AND NOT BUILD_PYTHON_BINDINGS)
  set(_coolbox_skip_pybind11 TRUE)
  set(_coolbox_skip_pybind11_reason "BUILD_PYTHON_BINDINGS=OFF")
endif()

if(WIN32)
  if(CMAKE_VS_PLATFORM_NAME STREQUAL "ARM64" OR CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
    set(_coolbox_skip_pybind11 TRUE)
    set(_coolbox_skip_pybind11_reason "Windows ARM64 target")
  endif()
endif()

if(_coolbox_skip_pybind11)
  if(_coolbox_skip_pybind11_reason STREQUAL "")
    message(STATUS "Skipping pybind11 FetchContent.")
  else()
    message(STATUS "Skipping pybind11 FetchContent (${_coolbox_skip_pybind11_reason}).")
  endif()
elseif(NOT TARGET pybind11::pybind11)
  FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.13.6
  )
  # Respect caller-provided -DPYBIND11_FINDPYTHON=... and only default to ON when not provided.
  if(NOT DEFINED PYBIND11_FINDPYTHON)
    set(PYBIND11_FINDPYTHON ON CACHE BOOL "Use FindPython instead of deprecated FindPythonInterp")
  endif()
  FetchContent_MakeAvailable(pybind11)
endif()
# Use FetchContent to manage external dependencies
include(FetchContent)


# --- Eigen (header-only, always fetch for portability) ---
if(NOT TARGET Eigen3::Eigen)
  message(STATUS "Fetching Eigen3 via FetchContent (header-only)...")
  include(FetchContent)
  FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3.4.0
  )

  # Populate Eigen sources without add_subdirectory/MakeAvailable so Eigen's
  # own tests are not registered into this repository's CTest run.
  FetchContent_GetProperties(eigen)
  if(NOT eigen_POPULATED)
    FetchContent_Populate(eigen)
  endif()

  # Provide Eigen3::Eigen target for consumers
  if(NOT TARGET Eigen3::Eigen)
    add_library(Eigen3::Eigen INTERFACE IMPORTED)
    set_target_properties(Eigen3::Eigen PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${eigen_SOURCE_DIR}"
    )
  endif()
endif()


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