#!/usr/bin/env bash
set -euo pipefail

# shellcheck source=common.sh
source /workspace/_local_build_pipeline/scripts/jobs/common.sh

ensure_local_build
ref_name="$(default_ref_name)"
stage_release_dir

export COOLBOX_LIB_DIR=/workspace/build

EMSDK_DIR=/tmp/emsdk-local
rm -rf "${EMSDK_DIR}" build-emscripten staging-js
git clone https://github.com/emscripten-core/emsdk.git "${EMSDK_DIR}"
cd "${EMSDK_DIR}"
./emsdk install latest
./emsdk activate latest --embedded
# shellcheck disable=SC1091
source ./emsdk_env.sh >/dev/null

cd /workspace
"${EMSDK_DIR}/upstream/emscripten/emcmake" cmake -S _libraries/emscripten_bindings -B build-emscripten -G Ninja -DCMAKE_BUILD_TYPE=Release -DEMSCRIPTEN_MODULARIZE=ON
cmake --build build-emscripten --config Release

mkdir -p staging-js
cp -R build-emscripten/. staging-js/ 2>/dev/null || true
tar -C staging-js -czf "release-assets/coolbox-js-bindings-linux-x86_64-${ref_name}.tar.gz" .
python3 -c 'import pathlib,sys,zipfile; s=pathlib.Path(sys.argv[1]); z=pathlib.Path(sys.argv[2]); a=zipfile.ZipFile(z,"w",compression=zipfile.ZIP_DEFLATED); [a.write(p,p.relative_to(s)) for p in sorted(s.rglob("*")) if p.is_file()]; a.close()' \
  staging-js "release-assets/coolbox-js-bindings-linux-x86_64-${ref_name}.zip"
