#!/usr/bin/env bash
set -euo pipefail

cd /workspace

rm -rf build
cmake -S . -B build -Wno-dev \
  -DBUILD_BINARIES=OFF \
  -DBUILD_PRODUCTS=ON \
  -DBUILD_PRODUCT_INSTALLER_ABSTRACTIONS=ON \
  -DBUILD_IO_SQL=ON \
  -DBUILD_TESTING=ON \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build --target build_libraries
ctest --test-dir build --output-on-failure
