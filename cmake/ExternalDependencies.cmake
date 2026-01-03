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


# Find or fetch Doxygen

find_package(Doxygen QUIET)
if(NOT DOXYGEN_FOUND)
  message(STATUS "Doxygen not found, fetching via FetchContent...")
  include(FetchContent)
endif()

# =============================
# Fetch quiche (QUIC/HTTP3)
# =============================
include(FetchContent)
FetchContent_Declare(
  quiche
  GIT_REPOSITORY https://github.com/cloudflare/quiche.git
  GIT_TAG 0.21.0
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/quiche
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(quiche)

find_package(GSL REQUIRED)