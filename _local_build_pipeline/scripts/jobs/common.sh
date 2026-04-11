#!/usr/bin/env bash
set -euo pipefail

cd /workspace

ensure_local_build() {
  if [ ! -d build ] || [ ! -f build/CMakeCache.txt ]; then
    bash ./_local_build_pipeline/scripts/jobs/build-libs.sh
  fi
}

ensure_python_env() {
  if [ ! -d .venv ]; then
    python3 -m venv .venv
  fi
  # shellcheck disable=SC1091
  . .venv/bin/activate
  python -m pip install --upgrade pip setuptools wheel build pybind11 numpy
}

default_ref_name() {
  printf '%s' "${GITHUB_REF_NAME:-local}"
}

stage_release_dir() {
  rm -rf release-assets
  mkdir -p release-assets
}
