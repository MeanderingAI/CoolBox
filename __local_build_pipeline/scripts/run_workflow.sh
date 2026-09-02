#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "${SCRIPT_DIR}/common.sh"

if [ "$#" -lt 1 ]; then
  cat <<'EOF' >&2
Usage: run_workflow.sh <workflow-name> [--rebuild-image] [--backend <docker-linux|native-windows|native-macos>] [extra args passed to act]

Examples:
  _local_build_pipeline/scripts/run_workflow.sh build-libs.yaml
  _local_build_pipeline/scripts/run_workflow.sh build-libs.yaml --backend native-windows
  _local_build_pipeline/scripts/run_workflow.sh build-libs.yaml --backend native-macos
  _local_build_pipeline/scripts/run_workflow.sh build-purchase-pipeline
  _local_build_pipeline/scripts/run_workflow.sh generate-purchase-python.yaml --act
EOF
  exit 1
fi

workflow="$1"
shift || true

rebuild_image=false
force_act=false
backend="docker-linux"
pass_through=()
while [ "$#" -gt 0 ]; do
  arg="$1"
  shift
  case "${arg}" in
    --rebuild-image)
      rebuild_image=true
      ;;
    --act)
      force_act=true
      ;;
    --backend)
      if [ "$#" -lt 1 ]; then
        echo "--backend requires a value" >&2
        exit 1
      fi
      backend="$1"
      shift
      ;;
    *)
      pass_through+=("${arg}")
      ;;
  esac
done

workflow="${workflow##*/}"

if [ "${rebuild_image}" = true ] && [ "${backend}" = "docker-linux" ]; then
  build_image
fi

native_windows_runner() {
  local script_path="$1"
  if command -v powershell >/dev/null 2>&1; then
    powershell -NoProfile -ExecutionPolicy Bypass -File "${script_path}"
  elif command -v pwsh >/dev/null 2>&1; then
    pwsh -NoProfile -File "${script_path}"
  else
    echo "powershell or pwsh is required for native-windows backend" >&2
    exit 1
  fi
}

native_macos_runner() {
  local script_path="$1"
  bash "${script_path}"
}

case "${workflow}" in
  build-libs|build-libs.yaml)
    case "${backend}" in
      docker-linux)
        docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/build-libs.sh
        ;;
      native-windows)
        native_windows_runner "${REPO_ROOT}/_local_build_pipeline/scripts/jobs/build-libs.windows.ps1"
        ;;
      native-macos)
        native_macos_runner "${REPO_ROOT}/_local_build_pipeline/scripts/jobs/build-libs.macos.sh"
        ;;
      *)
        echo "Unsupported backend '${backend}' for workflow '${workflow}'" >&2
        exit 2
        ;;
    esac
    ;;
  build-products|build-products.yaml)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/build-products.sh
    ;;
  build-tests)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/build-tests.sh
    ;;
  build-cpp-docs|build-cpp-docs.yaml)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/build-cpp-docs.sh
    ;;
  build-tutorials|build-tutorials.yaml)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/build-tutorials.sh
    ;;
  build-purchase-pipeline|build-purchase-pipeline.yaml)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/build-purchase-pipeline.sh
    ;;
  lsp-vlang|lsp-vlang.yaml)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/build-lsp-vlang.sh
    docker build -f "${REPO_ROOT}/apps/lsp/docker/Dockerfile.vlang" -t coolbox-local-plvlang-lsp:test "${REPO_ROOT}/apps/lsp"
    ;;
  lsp-c3|lsp-c3.yaml)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/build-lsp-c3.sh
    docker build -f "${REPO_ROOT}/apps/lsp/docker/Dockerfile.c3" -t coolbox-local-plc3-lsp:test "${REPO_ROOT}/apps/lsp"
    ;;
  generate-purchase-c|generate-purchase-c.yaml)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/generate-purchase-c.sh
    ;;
  generate-purchase-python|generate-purchase-python.yaml)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/generate-purchase-python.sh
    ;;
  generate-purchase-go|generate-purchase-go.yaml)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/generate-purchase-go.sh
    ;;
  generate-purchase-js|generate-purchase-js.yaml)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/generate-purchase-js.sh
    ;;
  generate-purchase-v|generate-purchase-v.yaml)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/generate-purchase-v.sh
    ;;
  generate-purchase-c3|generate-purchase-c3.yaml)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/generate-purchase-c3.sh
    ;;
  generate-purchase-rust|generate-purchase-rust.yaml)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/generate-purchase-rust.sh
    ;;
  generate-purchase-r|generate-purchase-r.yaml)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/generate-purchase-r.sh
    ;;
  generate-purchase-java|generate-purchase-java.yaml)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/generate-purchase-java.sh
    ;;
  docs-publish|docs-publish.yaml)
    if [ "${backend}" != "docker-linux" ]; then
      echo "Workflow '${workflow}' currently supports only --backend docker-linux" >&2
      exit 2
    fi
    docker_run_repo bash /workspace/_local_build_pipeline/scripts/jobs/docs-publish-dry-run.sh
    ;;
  *)
    if [ "${force_act}" = true ]; then
      "${SCRIPT_DIR}/run_with_act.sh" "${workflow}" "${pass_through[@]}"
      exit 0
    fi
    cat <<EOF >&2
Workflow '${workflow}' does not have a dedicated local runner for backend '${backend}'.
Use:
  _local_build_pipeline/scripts/run_workflow.sh ${workflow} --act

This is typical for workflows that depend on GitHub artifacts, workflow_run triggers, or registry/release publishing.
EOF
    exit 2
    ;;
esac
