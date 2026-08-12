#!/usr/bin/env bash
set -euo pipefail

SCRIPT_NAME="$(basename "$0")"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_DIR="${1:-${PROJECT_ROOT}/cpp-docs-site}"

if ! command -v doxygen >/dev/null 2>&1; then
  echo "[${SCRIPT_NAME}] ERROR: doxygen not found on PATH" >&2
  exit 1
fi

INPUTS=()
for path in \
  "${PROJECT_ROOT}/_libraries/include" \
  "${PROJECT_ROOT}/_libraries/packages" \
  "${PROJECT_ROOT}/_deliverables/libraries"; do
  if [ -d "${path}" ]; then
    INPUTS+=("${path}")
  fi
done

if [ ${#INPUTS[@]} -eq 0 ]; then
  echo "[${SCRIPT_NAME}] ERROR: no documentation input directories found" >&2
  exit 1
fi

echo "[${SCRIPT_NAME}] Building C++ docs into ${OUT_DIR}"
rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

DOXYFILE_TMP="${OUT_DIR}/Doxyfile.generated"
cat > "${DOXYFILE_TMP}" <<EOF
PROJECT_NAME           = "CoolBox C++ API"
OUTPUT_DIRECTORY       = ${OUT_DIR}
INPUT                  = ${INPUTS[*]}
RECURSIVE              = YES
FILE_PATTERNS          = *.h *.hpp
GENERATE_HTML          = YES
HTML_OUTPUT            = .
GENERATE_LATEX         = NO
EXTRACT_ALL            = YES
EXCLUDE_PATTERNS       = */build/* */_deps/* */googletest-*/* */eigen-*/*
QUIET                  = YES
WARN_IF_UNDOCUMENTED   = NO
WARN_IF_DOC_ERROR      = YES
EOF

doxygen "${DOXYFILE_TMP}"
rm -f "${DOXYFILE_TMP}"

echo "[${SCRIPT_NAME}] ✓ C++ docs generated."
