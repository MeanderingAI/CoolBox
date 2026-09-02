#!/usr/bin/env bash
set -euo pipefail

# shellcheck source=common.sh
source /workspace/_local_build_pipeline/scripts/jobs/common.sh

ref_name="$(default_ref_name)"
stage_release_dir

rm -rf _libraries/c_bindings/build staging-v .prebuilt/v-native
mkdir -p staging-v/module staging-v/native/include staging-v/native/lib .prebuilt/v-native/lib

cmake -S _libraries/c_bindings -B _libraries/c_bindings/build -DBUILD_TESTING=ON
cmake --build _libraries/c_bindings/build --config Release
ctest --test-dir _libraries/c_bindings/build --output-on-failure

cp -R /workspace/_libraries/vlang_bindings/. staging-v/module/
cp -R /workspace/_libraries/c_bindings/include/. staging-v/native/include/
find /workspace/_libraries/c_bindings/build -maxdepth 1 -type f \( -name 'libcoolbox_c_bindings.*' -o -name 'coolbox_c_bindings.lib' -o -name 'coolbox_c_bindings.dll' \) -exec cp {} staging-v/native/lib/ \;
cp -R staging-v/native/lib/. .prebuilt/v-native/lib/ 2>/dev/null || true

if command -v v >/dev/null 2>&1; then
  export COOLBOX_C_BINDINGS_DIR=/workspace/.prebuilt/v-native/lib
  (cd /workspace/_libraries/vlang_bindings && v test .)
else
  echo "V compiler not installed in local Linux CI image; packaging sources and native dependency bundle without smoke tests."
fi

tar -C staging-v -czf "$(package_tarball_path v-bindings)" .
create_zip_from_dir staging-v "$(package_zip_path v-bindings)"
finalize_release_assets

echo "Generated V purchase package for ${ref_name}"