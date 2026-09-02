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

cmake --build build --target plvlang_lsp plvlang_lsp_test -- -j1
ctest --test-dir build -R plvlang_lsp_test --output-on-failure
mkdir -p apps/lsp/dist
cp build/apps/lsp/plvlang_lsp apps/lsp/dist/plvlang_lsp 2>/dev/null || cp build/plvlang_lsp apps/lsp/dist/plvlang_lsp