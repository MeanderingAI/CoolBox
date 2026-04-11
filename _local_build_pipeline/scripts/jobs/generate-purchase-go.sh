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
tar -C staging-go -czf "release-assets/coolbox-go-bindings-linux-x86_64-${ref_name}.tar.gz" .
python3 -c 'import pathlib,sys,zipfile; s=pathlib.Path(sys.argv[1]); z=pathlib.Path(sys.argv[2]); a=zipfile.ZipFile(z,"w",compression=zipfile.ZIP_DEFLATED); [a.write(p,p.relative_to(s)) for p in sorted(s.rglob("*")) if p.is_file()]; a.close()' \
  staging-go "release-assets/coolbox-go-bindings-linux-x86_64-${ref_name}.zip"
