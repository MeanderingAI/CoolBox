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
tar -C staging-js -czf "$(package_tarball_path js-bindings)" .
create_zip_from_dir staging-js "$(package_zip_path js-bindings)"
finalize_release_assets
