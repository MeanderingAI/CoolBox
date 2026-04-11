#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PIPELINE_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(cd "${PIPELINE_ROOT}/.." && pwd)"
DEFAULT_IMAGE="coolbox-local-linux-ci:latest"

image_name() {
  printf '%s' "${COOLBOX_LOCAL_IMAGE:-${DEFAULT_IMAGE}}"
}

ensure_docker() {
  if ! command -v docker >/dev/null 2>&1; then
    echo "docker is required for _local_build_pipeline scripts" >&2
    exit 1
  fi
}

ensure_act() {
  if ! command -v act >/dev/null 2>&1; then
    echo "act is required for this workflow mode" >&2
    exit 1
  fi
}

build_image() {
  ensure_docker
  local image
  image="$(image_name)"
  docker build -t "${image}" -f "${PIPELINE_ROOT}/docker/linux-ci.Dockerfile" "${REPO_ROOT}"
}

docker_run_repo() {
  ensure_docker
  local image
  local job_name
  image="$(image_name)"
  job_name="docker-run"
  for arg in "$@"; do
    if [[ "${arg}" == *.sh ]]; then
      job_name="$(basename "${arg}" .sh)"
    fi
  done
  docker run --rm -t \
    -e CI=true \
    -e GITHUB_ACTIONS=false \
    -e GITHUB_WORKSPACE=/workspace \
    -e COOLBOX_LOCAL_JOB_NAME="${job_name}" \
    -v "${REPO_ROOT}:/repo" \
    -w /workspace \
    "${image}" bash /repo/_local_build_pipeline/scripts/container_entrypoint.sh "${job_name}" "$@"
}
