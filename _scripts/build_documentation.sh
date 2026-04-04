#!/usr/bin/env bash
# build_documentation.sh — Build the full documentation site.
#
# Usage:
#   bash _scripts/build_documentation.sh [PROJECT_ROOT]
#
# PROJECT_ROOT defaults to the current working directory.

set -euo pipefail

SCRIPT_NAME="$(basename "$0")"
PROJECT_ROOT="${1:-$(pwd)}"

# Resolve Python executable
if [ -x "${PROJECT_ROOT}/.venv/bin/python" ]; then
  PYTHON="${PROJECT_ROOT}/.venv/bin/python"
else
  PYTHON="python3"
fi

SITE_DIR="${PROJECT_ROOT}/.site"
TUTORIALS_DIR="${SITE_DIR}/tutorials"

echo "[${SCRIPT_NAME}] Building documentation from tutorials, publications, and references..."

# 1. Build tutorials directly into .site/tutorials/
echo "[${SCRIPT_NAME}] Running build_tutorials.py → ${TUTORIALS_DIR}"
"${PYTHON}" "${PROJECT_ROOT}/_scripts/build_tutorials.py" \
  "${PROJECT_ROOT}/tutorials" \
  "${TUTORIALS_DIR}"

# Report tutorials/index.html
if [ -f "${TUTORIALS_DIR}/index.html" ]; then
  echo "[${SCRIPT_NAME}] Written tutorials/index.html → ${TUTORIALS_DIR}/index.html"
fi

# Build publications and references content from bib files
# This replaces the previous tutorial-generated publications path.
"${PYTHON}" "${PROJECT_ROOT}/_scripts/build_publications.py" --bib "${PROJECT_ROOT}/bib" --out "${SITE_DIR}"

# Move tutorial tag pages up to .site/tags/ (will be merged with build_tags output)
if [ -d "${TUTORIALS_DIR}/tags" ]; then
  mkdir -p "${SITE_DIR}/tags"
  cp -a "${TUTORIALS_DIR}/tags/"* "${SITE_DIR}/tags/" 2>/dev/null || true
  echo "[${SCRIPT_NAME}] Copied tutorial tag pages → ${SITE_DIR}/tags"
  rm -rf "${TUTORIALS_DIR}/tags"
  echo "[${SCRIPT_NAME}] Removed tutorial tags folder to avoid duplicate structure"
fi

# 2. Generate the docs hub portal
echo "[${SCRIPT_NAME}] Running generate_docs_hub.sh → ${SITE_DIR}"
DOCS_TUTORIALS_DIR="${TUTORIALS_DIR}" \
  bash "${PROJECT_ROOT}/_scripts/generate_docs_hub.sh" "${SITE_DIR}"

# 3. Build references into .site/references/
echo "[${SCRIPT_NAME}] Running build_references.py → ${SITE_DIR}/references"
"${PYTHON}" "${PROJECT_ROOT}/_scripts/build_references.py" --bib "${PROJECT_ROOT}/bib" --out "${SITE_DIR}/references"

# 4. Build tag pages into .site/tags/
echo "[${SCRIPT_NAME}] Running build_tags.py → ${SITE_DIR}/tags"
"${PYTHON}" "${PROJECT_ROOT}/_scripts/build_tags.py" --out "${SITE_DIR}/tags"

# Report generated tag pages
if [ -d "${SITE_DIR}/tags" ]; then
  echo "[${SCRIPT_NAME}] Tag pages generated:"
  find "${SITE_DIR}/tags" -name '*.html' -print | sort | while read -r f; do
    echo "  -> ${f}"
  done
  if [ -f "${SITE_DIR}/tags/index.html" ]; then
    echo "[${SCRIPT_NAME}] Written tags/index.html → ${SITE_DIR}/tags/index.html"
  fi
fi

echo "[${SCRIPT_NAME}] ✓ Documentation site generated."

# Safety: If some earlier step wrote docs to an alternate location, normalize
# common variants into the expected `.site` directory so CI and publishing
# always see a consistent output path.
if [ ! -d "${SITE_DIR}" ] || [ -z "$(ls -A "${SITE_DIR}" 2>/dev/null)" ]; then
  echo "[${SCRIPT_NAME}] .site is empty; searching for alternate output locations"
  for alt in "build/documentation_site" "build/documentation" ".documentation" "documentation" "docs_output"; do
    if [ -d "${PROJECT_ROOT}/${alt}" ] && [ "$(ls -A "${PROJECT_ROOT}/${alt}" 2>/dev/null)" ]; then
      echo "[${SCRIPT_NAME}] Moving ${alt} -> .site"
      rm -rf "${SITE_DIR}" || true
      mv "${PROJECT_ROOT}/${alt}" "${SITE_DIR}"
      break
    fi
  done
fi
