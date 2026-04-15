#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_DIR="${1:-${ROOT_DIR}/cpp-docs-site}"
PROJECT_NAME="${CPP_DOCS_PROJECT_NAME:-CoolBox C++ API}"

if ! command -v doxygen >/dev/null 2>&1; then
  echo "doxygen is required to build the C++ documentation." >&2
  exit 1
fi

cpp_inputs=()
if [ -d "${ROOT_DIR}/_libraries/include" ]; then
  cpp_inputs+=("${ROOT_DIR}/_libraries/include")
fi
if [ -d "${ROOT_DIR}/_libraries/packages" ]; then
  cpp_inputs+=("${ROOT_DIR}/_libraries/packages")
fi

if [ ${#cpp_inputs[@]} -eq 0 ]; then
  echo "No C++ header roots found for Doxygen." >&2
  exit 1
fi

rm -rf "${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}"

DOXYFILE_PATH="${OUTPUT_DIR}/Doxyfile"
trap 'rm -f "${DOXYFILE_PATH}"' EXIT

{
  printf 'PROJECT_NAME = "%s"\n' "${PROJECT_NAME}"
  printf 'OUTPUT_DIRECTORY = %s\n' "${OUTPUT_DIR}"
  printf 'INPUT ='
  for input_dir in "${cpp_inputs[@]}"; do
    printf ' "%s"' "${input_dir}"
  done
  printf '\n'
  cat <<'EOF'
RECURSIVE = YES
FILE_PATTERNS = *.h *.hpp
GENERATE_HTML = YES
HTML_OUTPUT = .
GENERATE_LATEX = NO
EXTRACT_ALL = YES
EXCLUDE_PATTERNS = */build/* */_deps/* */googletest-*/* */eigen-*/*
QUIET = YES
WARN_IF_UNDOCUMENTED = NO
WARN_IF_DOC_ERROR = YES
EOF
} > "${DOXYFILE_PATH}"

doxygen "${DOXYFILE_PATH}"

if [ ! -f "${OUTPUT_DIR}/index.html" ]; then
  echo "Doxygen completed but did not produce ${OUTPUT_DIR}/index.html." >&2
  exit 1
fi

echo "C++ documentation generated in ${OUTPUT_DIR}"