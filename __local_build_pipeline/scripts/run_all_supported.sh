#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

"${SCRIPT_DIR}/run_workflow.sh" build-libs.yaml "$@"
"${SCRIPT_DIR}/run_workflow.sh" build-tests "$@"
"${SCRIPT_DIR}/run_workflow.sh" build-products.yaml "$@"
"${SCRIPT_DIR}/run_workflow.sh" build-cpp-docs.yaml "$@"
"${SCRIPT_DIR}/run_workflow.sh" build-tutorials.yaml "$@"
"${SCRIPT_DIR}/run_workflow.sh" generate-purchase-c.yaml "$@"
"${SCRIPT_DIR}/run_workflow.sh" generate-purchase-python.yaml "$@"
"${SCRIPT_DIR}/run_workflow.sh" generate-purchase-go.yaml "$@"
"${SCRIPT_DIR}/run_workflow.sh" generate-purchase-js.yaml "$@"
"${SCRIPT_DIR}/run_workflow.sh" generate-purchase-rust.yaml "$@"
"${SCRIPT_DIR}/run_workflow.sh" generate-purchase-r.yaml "$@"
"${SCRIPT_DIR}/run_workflow.sh" generate-purchase-java.yaml "$@"
"${SCRIPT_DIR}/run_workflow.sh" build-purchase-pipeline.yaml "$@"
