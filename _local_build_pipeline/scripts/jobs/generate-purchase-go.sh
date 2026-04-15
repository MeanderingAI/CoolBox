#!/usr/bin/env bash
set -euo pipefail

# shellcheck source=common.sh
source /workspace/_local_build_pipeline/scripts/jobs/common.sh

ensure_local_build
ref_name="$(default_ref_name)"
stage_release_dir

export COOLBOX_LIB_DIR=/workspace/build

mkdir -p build/test-logs
cd /workspace/_libraries/go_bindings
go test -count=1 -v ./... | tee /workspace/build/test-logs/go-bindings-linux-x86_64.log

cd /workspace
rm -rf staging-go
mkdir -p staging-go
cp -R _libraries/go_bindings/. staging-go/
rm -rf staging-go/.git staging-go/.github
tar -C staging-go -czf "$(package_tarball_path go-bindings)" .
create_zip_from_dir staging-go "$(package_zip_path go-bindings)"
finalize_release_assets
