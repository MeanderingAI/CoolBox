#!/usr/bin/env bash
set -euo pipefail

cd /workspace

rm -rf build release-assets
cmake -S . -B build -Wno-dev \
  -DBUILD_BINARIES=OFF \
  -DBUILD_PRODUCTS=ON \
  -DBUILD_PRODUCT_INSTALLER_ABSTRACTIONS=ON \
  -DBUILD_IO_SQL=ON \
  -DBUILD_TESTING=OFF \
  -DCMAKE_BUILD_TYPE=Release

help_out="$(cmake --build build --target help 2>&1 || true)"
products=(MStudio file_browser bower_shell)
for product in "${products[@]}"; do
  if echo "${help_out}" | grep -Eq "(^|[[:space:]])${product}([[:space:]]|$)"; then
    echo "Building product target: ${product}"
    cmake --build build --target "${product}" --config Release
  else
    echo "Skipping missing product target: ${product}"
  fi
done

mkdir -p release-assets
find build -maxdepth 5 -type f \( -name 'MStudio' -o -name 'file_browser' -o -name 'bower_shell' -o -name '*.so' -o -name '*.a' \) -print
