#!/usr/bin/env bash
set -euo pipefail

cd /workspace

echo "[local-build-pipeline] Running Docker-local subset of build-purchase-pipeline"
echo "[local-build-pipeline] This executes the locally meaningful Linux jobs only"

bash ./_local_build_pipeline/scripts/jobs/build-libs.sh
bash ./_local_build_pipeline/scripts/jobs/build-products.sh
bash ./_local_build_pipeline/scripts/jobs/build-cpp-docs.sh
bash ./_local_build_pipeline/scripts/jobs/build-tutorials.sh

echo "[local-build-pipeline] Skipped GitHub artifact fan-out, registry push, and Pages publish stages"
