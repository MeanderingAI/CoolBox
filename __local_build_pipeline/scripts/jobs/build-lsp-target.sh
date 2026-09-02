#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 3 ]; then
  echo "Usage: build-lsp-target.sh <binary-target> <test-target> <binary-name>" >&2
  exit 2
fi

binary_target="$1"
test_target="$2"
binary_name="$3"
host_out="${HOST_OUT:-}"

write_status() {
  local status="$1"
  if [ -n "${host_out}" ]; then
    mkdir -p "${host_out}"
    printf 'EXIT=%s\n' "${status}" > "${host_out}/exit.txt"
  fi
}

trap 'write_status "$?"' EXIT

rm -rf build
cmake -S . -B build -Wno-dev \
  -DBUILD_BINARIES=OFF \
  -DBUILD_PRODUCTS=ON \
  -DBUILD_PRODUCT_INSTALLER_ABSTRACTIONS=ON \
  -DBUILD_IO_SQL=ON \
  -DBUILD_TESTING=ON \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build --target "${binary_target}" "${test_target}" -- -j1
ctest --test-dir build -R "${test_target}" --output-on-failure

if [ -n "${host_out}" ]; then
  cp "build/apps/lsp/${binary_name}" "${host_out}/${binary_name}" 2>/dev/null || cp "build/${binary_name}" "${host_out}/${binary_name}"
fi