#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

mkdir -p "${SCRIPT_DIR}/target"
(
  cd "${SCRIPT_DIR}"
  doxygen Doxyfile
)
