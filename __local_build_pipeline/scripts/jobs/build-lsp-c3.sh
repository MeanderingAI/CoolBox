#!/usr/bin/env bash
set -euo pipefail

cd /workspace

rm -rf build apps/lsp/dist
cmake -S . -B build -Wno-dev \
  -DBUILD_BINARIES=OFF \
  -DBUILD_PRODUCTS=ON \
  -DBUILD_PRODUCT_INSTALLER_ABSTRACTIONS=ON \
  -DBUILD_IO_SQL=ON \
  -DBUILD_TESTING=ON \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build --target plc3_lsp plc3_lsp_test -- -j1
ctest --test-dir build -R plc3_lsp_test --output-on-failure
mkdir -p apps/lsp/dist
cp build/apps/lsp/plc3_lsp apps/lsp/dist/plc3_lsp 2>/dev/null || cp build/plc3_lsp apps/lsp/dist/plc3_lsp