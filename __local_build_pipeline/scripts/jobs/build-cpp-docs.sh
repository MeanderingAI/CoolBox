#!/usr/bin/env bash
set -euo pipefail

cd /workspace
rm -rf .local-build/cpp-docs-site
mkdir -p .local-build
bash ./_scripts/build_scripts/build_cpp_docs.sh "/workspace/.local-build/cpp-docs-site"
