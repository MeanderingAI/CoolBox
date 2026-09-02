#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "${SCRIPT_DIR}/common.sh"

if [ "$#" -lt 1 ]; then
  echo "Usage: run_with_act.sh <workflow-name> [act args...]" >&2
  exit 1
fi

ensure_act

workflow="$1"
shift || true
workflow="${workflow##*/}"

workflow_path="${REPO_ROOT}/.github/workflows/${workflow}"
if [ ! -f "${workflow_path}" ]; then
  echo "Workflow not found: ${workflow_path}" >&2
  exit 1
fi

exec act workflow_dispatch -W "${workflow_path}" "$@"
