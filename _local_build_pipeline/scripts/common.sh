#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PIPELINE_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(cd "${PIPELINE_ROOT}/.." && pwd)"
DEFAULT_IMAGE="coolbox-local-linux-ci:latest"

is_windows_host() {
  case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

normalize_host_path() {
  local raw_path="$1"
  if is_windows_host && command -v cygpath >/dev/null 2>&1; then
    cygpath -m "${raw_path}"
  else
    printf '%s' "${raw_path}"
  fi
}

docker_output_root() {
  if [ -n "${COOLBOX_LOCAL_HOST_OUT_ROOT:-}" ]; then
    normalize_host_path "${COOLBOX_LOCAL_HOST_OUT_ROOT}"
    return
  fi

  if is_windows_host; then
    local tmp_root
    tmp_root="${TEMP:-${TMP:-/tmp}}"
    normalize_host_path "${tmp_root}/coolbox-local-pipeline-out"
    return
  fi

  printf '%s' "${REPO_ROOT}/_local_build_pipeline/out"
}

ensure_host_directory() {
  local host_path="$1"
  if is_windows_host && command -v cygpath >/dev/null 2>&1; then
    mkdir -p "$(cygpath -u "${host_path}")"
  else
    mkdir -p "${host_path}"
  fi
}

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
  local host_output_root
  image="$(image_name)"
  job_name="docker-run"
  for arg in "$@"; do
    if [[ "${arg}" == *.sh ]]; then
      job_name="$(basename "${arg}" .sh)"
    fi
  done
  host_output_root="$(docker_output_root)"
  ensure_host_directory "${host_output_root}"
  MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*' docker run --rm -t \
    -e CI=true \
    -e GITHUB_ACTIONS=false \
    -e GITHUB_WORKSPACE=/workspace \
    -e COOLBOX_LOCAL_JOB_NAME="${job_name}" \
    -e COOLBOX_LOCAL_OUTPUT_ROOT=/host_out \
    -v "${REPO_ROOT}:/repo" \
    -v "${host_output_root}:/host_out" \
    -w /workspace \
    "${image}" bash /repo/_local_build_pipeline/scripts/container_entrypoint.sh "${job_name}" "$@"
}
