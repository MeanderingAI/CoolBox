#!/usr/bin/env bash
set -euo pipefail

cd /workspace
python3 -m venv .venv
. .venv/bin/activate
python -m pip install --upgrade pip

rm -rf .prebuilt .site
mkdir -p .prebuilt/cpp-docs .prebuilt/tutorials

bash ./_scripts/build_cpp_docs.sh "/workspace/.prebuilt/cpp-docs"
python ./_scripts/build_tags.py --out ./.prebuilt/tutorials/tags
python ./_scripts/build_publications.py --bib ./bib --out ./.prebuilt/tutorials
python ./_scripts/build_tutorials.py ./tutorials ./.prebuilt/tutorials

echo "[local-build-pipeline] Dry-run completed. This does not push to Pages or resolve GitHub release artifacts."
