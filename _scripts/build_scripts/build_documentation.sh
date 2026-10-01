#!/usr/bin/env bash
set -euo pipefail

SCRIPT_NAME="$(basename "$0")"
PROJECT_ROOT="${1:-$(pwd)}"
SITE_DIR="${PROJECT_ROOT}/.site"
TUTORIAL_SOURCE_DIR="${PROJECT_ROOT}/__init__/tutorials"

if [ ! -d "${TUTORIAL_SOURCE_DIR}" ] && [ -d "${PROJECT_ROOT}/tutorials" ]; then
  TUTORIAL_SOURCE_DIR="${PROJECT_ROOT}/tutorials"
fi

if [ -x "${PROJECT_ROOT}/.venv/bin/python" ]; then
  PYTHON="${PROJECT_ROOT}/.venv/bin/python"
else
  PYTHON="python3"
fi

echo "[${SCRIPT_NAME}] Building tutorials site from ${TUTORIAL_SOURCE_DIR} → ${SITE_DIR}"
rm -rf "${SITE_DIR}"
mkdir -p "${SITE_DIR}"
"${PYTHON}" "${PROJECT_ROOT}/_scripts/build_scripts/build_tutorials.py" "${TUTORIAL_SOURCE_DIR}" "${SITE_DIR}"

echo "[${SCRIPT_NAME}] ✓ Documentation site generated."
