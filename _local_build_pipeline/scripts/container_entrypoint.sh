#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 2 ]; then
  echo "Usage: container_entrypoint.sh <job-name> <command> [args...]" >&2
  exit 1
fi

job_name="$1"
shift

cd /
rm -rf /workspace
mkdir -p /workspace "/repo/_local_build_pipeline/out/${job_name}"

tar \
  --exclude='./.git' \
  --exclude='./build' \
  --exclude='./_local_build_pipeline/out' \
  --exclude='./.venv' \
  -cf - -C /repo . | tar -xf - -C /workspace

cd /workspace
set +e
"$@" > "/repo/_local_build_pipeline/out/${job_name}/command.log" 2>&1
status=$?
set -e

printf 'EXIT_STATUS=%s\n' "$status" >> "/repo/_local_build_pipeline/out/${job_name}/command.log"

for path in release-assets .site .prebuilt cpp-docs-site build/Testing build/test-logs build-emscripten target/package _libraries/c_bindings/build _libraries/java_bindings/target; do
  if [ -e "/workspace/$path" ]; then
    rm -rf "/repo/_local_build_pipeline/out/${job_name}/$(basename "$path")"
    cp -a "/workspace/$path" "/repo/_local_build_pipeline/out/${job_name}/"
  fi
done

exit "$status"