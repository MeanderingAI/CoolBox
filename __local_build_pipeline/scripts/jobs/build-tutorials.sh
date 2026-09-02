#!/usr/bin/env bash
set -euo pipefail

cd /workspace
python3 -m venv .venv
. .venv/bin/activate
python -m pip install --upgrade pip
bash ./_scripts/build_documentation.sh "/workspace"
