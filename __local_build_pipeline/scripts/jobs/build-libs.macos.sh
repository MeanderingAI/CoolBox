#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
OUT_DIR="${REPO_ROOT}/_local_build_pipeline/out/build-libs-native-macos"

mkdir -p "${OUT_DIR}"
exec > >(tee "${OUT_DIR}/command.log") 2>&1

status=0

sync_if_exists() {
  local path="$1"
  if [ -e "${REPO_ROOT}/${path}" ]; then
    rm -rf "${OUT_DIR}/$(basename "${path}")"
    cp -a "${REPO_ROOT}/${path}" "${OUT_DIR}/"
  fi
}

set +e
{
  cd "${REPO_ROOT}"

  if ! command -v brew >/dev/null 2>&1; then
    echo "Homebrew is required for the native-macos backend." >&2
    exit 1
  fi

  for pkg in eigen googletest gsl doxygen gcc sqlite; do
    brew list "$pkg" &>/dev/null || brew install "$pkg"
  done

  sqlite_prefix="$(brew --prefix sqlite)"

  rm -rf build
  cmake -S . -B build -Wno-dev \
    -DBUILD_BINARIES=OFF \
    -DBUILD_PRODUCTS=ON \
    -DBUILD_PRODUCT_INSTALLER_ABSTRACTIONS=ON \
    -DBUILD_IO_SQL=ON \
    -DBUILD_TESTING=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DSQLITE3_ROOT="${sqlite_prefix}" \
    -DSQLite3_ROOT="${sqlite_prefix}" \
    -DCMAKE_PREFIX_PATH="${sqlite_prefix}"

  make build_libraries
  make test
} 
status=$?
set -e

printf 'EXIT_STATUS=%s\n' "${status}"

sync_if_exists "build/Testing"
sync_if_exists "build/test-logs"
sync_if_exists "release-assets"

exit "${status}"