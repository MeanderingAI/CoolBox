#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "${SCRIPT_DIR}/common.sh"

if [ "$#" -lt 1 ]; then
  cat <<'EOF' >&2
Usage: run_workflow.sh <workflow-name> [--rebuild-image] [extra args passed to act]

Examples:
  _local_build_pipeline/scripts/run_workflow.sh build-libs.yaml
  _local_build_pipeline/scripts/run_workflow.sh build-purchase-pipeline
  _local_build_pipeline/scripts/run_workflow.sh generate-purchase-python.yaml --act
EOF
  exit 1
fi

workflow="$1"
shift || true

rebuild_image=false
force_act=false
pass_through=()
for arg in "$@"; do
  case "${arg}" in
    --rebuild-image)
      rebuild_image=true
      ;;
    --act)
      force_act=true
      ;;
    *)
      pass_through+=("${arg}")
      ;;
  esac
done

workflow="${workflow##*/}"

if [ "${rebuild_image}" = true ]; then
  build_image
fi

case "${workflow}" in
  build-libs|build-libs.yaml)
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/build-libs.sh
    ;;
  build-products|build-products.yaml)
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/build-products.sh
    ;;
  build-tests)
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/build-tests.sh
    ;;
  build-cpp-docs|build-cpp-docs.yaml)
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/build-cpp-docs.sh
    ;;
  build-tutorials|build-tutorials.yaml)
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/build-tutorials.sh
    ;;
  build-purchase-pipeline|build-purchase-pipeline.yaml)
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/build-purchase-pipeline.sh
    ;;
  lsp-vlang|lsp-vlang.yaml)
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/build-lsp-vlang.sh
    docker build -f "${REPO_ROOT}/apps/lsp/docker/Dockerfile.vlang" -t coolbox-local-plvlang-lsp:test "${REPO_ROOT}/apps/lsp"
    ;;
  lsp-c3|lsp-c3.yaml)
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/build-lsp-c3.sh
    docker build -f "${REPO_ROOT}/apps/lsp/docker/Dockerfile.c3" -t coolbox-local-plc3-lsp:test "${REPO_ROOT}/apps/lsp"
    ;;
  generate-purchase-c|generate-purchase-c.yaml)
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/generate-purchase-c.sh
    ;;
  generate-purchase-python|generate-purchase-python.yaml)
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/generate-purchase-python.sh
    ;;
  generate-purchase-go|generate-purchase-go.yaml)
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/generate-purchase-go.sh
    ;;
  generate-purchase-js|generate-purchase-js.yaml)
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/generate-purchase-js.sh
    ;;
  generate-purchase-rust|generate-purchase-rust.yaml)
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/generate-purchase-rust.sh
    ;;
  generate-purchase-r|generate-purchase-r.yaml)
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/generate-purchase-r.sh
    ;;
  generate-purchase-java|generate-purchase-java.yaml)
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/generate-purchase-java.sh
    ;;
  docs-publish|docs-publish.yaml)
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/docs-publish-dry-run.sh
    ;;
  *)
    if [ "${force_act}" = true ]; then
      "${SCRIPT_DIR}/run_with_act.sh" "${workflow}" "${pass_through[@]}"
      exit 0
    fi
    cat <<EOF >&2
Workflow '${workflow}' does not have a dedicated Docker local runner.
Use:
  _local_build_pipeline/scripts/run_workflow.sh ${workflow} --act

This is typical for workflows that depend on GitHub artifacts, workflow_run triggers, or registry/release publishing.
EOF
    exit 2
    ;;
esac
