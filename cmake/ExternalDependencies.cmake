# --- pybind11 (for Python bindings) ---
include(FetchContent)
set(_coolbox_skip_pybind11 FALSE)
if(WIN32)
  if(CMAKE_VS_PLATFORM_NAME STREQUAL "ARM64" OR CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
    # Visual Studio's -A ARM64 does not necessarily set CMAKE_CROSSCOMPILING.
    if(CMAKE_CROSSCOMPILING OR NOT CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
      set(_coolbox_skip_pybind11 TRUE)
    endif()
  endif()
endif()

if(WIN32 AND NOT _coolbox_skip_pybind11 AND NOT Python_EXECUTABLE)
  # An x64 build on an ARM64 host needs an x64 Python; probing the host's ARM64
  # Python can fail or hang, so require an explicit interpreter in this case.
  # CMake may itself run under x64 emulation, so query the native architecture.
  set(_coolbox_native_arch "${CMAKE_HOST_SYSTEM_PROCESSOR}")
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    cmake_host_system_information(RESULT _coolbox_native_arch_reg
      QUERY WINDOWS_REGISTRY "HKLM/SYSTEM/CurrentControlSet/Control/Session Manager/Environment"
      VALUE PROCESSOR_ARCHITECTURE)
    if(_coolbox_native_arch_reg)
      set(_coolbox_native_arch "${_coolbox_native_arch_reg}")
    endif()
  endif()
  if("$ENV{PROCESSOR_IDENTIFIER}" MATCHES "^ARMv8")
    set(_coolbox_native_arch "ARM64")
  endif()
  if(_coolbox_native_arch MATCHES "^(ARM64|arm64|aarch64)$"
     AND (CMAKE_VS_PLATFORM_NAME MATCHES "^(x64|X64)$" OR CMAKE_GENERATOR_PLATFORM MATCHES "^(x64|X64)$"))
    set(_coolbox_skip_pybind11 TRUE)
    set(_coolbox_skip_pybind11_reason "x64 build on an ARM64 host; pass -DPython_EXECUTABLE=<x64 python.exe> to enable them")
  endif()
endif()

if(_coolbox_skip_pybind11_reason)
  message(WARNING "Skipping pybind11 and Python bindings: ${_coolbox_skip_pybind11_reason}.")
elseif(_coolbox_skip_pybind11)
  message(STATUS "Skipping pybind11 FetchContent for Windows ARM64 cross-compile.")
elseif(NOT TARGET pybind11::pybind11)
  FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v3.0.1
  )
  # Set PYBIND11_FINDPYTHON to use modern FindPython instead of deprecated FindPythonInterp
  set(PYBIND11_FINDPYTHON ON CACHE BOOL "Use FindPython instead of deprecated FindPythonInterp")
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
    # Eigen is consumed as headers only. A deliberately absent source
    # subdirectory prevents its project and tests from being added.
    SOURCE_SUBDIR _coolbox_header_only
  )

  FetchContent_MakeAvailable(eigen)

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