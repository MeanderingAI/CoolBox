#!/usr/bin/env bash
set -euo pipefail

# shellcheck source=common.sh
source /workspace/_local_build_pipeline/scripts/jobs/common.sh

ref_name="$(default_ref_name)"
stage_release_dir

rm -rf _libraries/c_bindings/build staging-c3 .prebuilt/c3-native
mkdir -p staging-c3/module staging-c3/native/include staging-c3/native/lib .prebuilt/c3-native/lib

cmake -S _libraries/c_bindings -B _libraries/c_bindings/build -DBUILD_TESTING=ON
cmake --build _libraries/c_bindings/build --config Release
ctest --test-dir _libraries/c_bindings/build --output-on-failure

cp -R /workspace/_libraries/c3_bindings/. staging-c3/module/
cp -R /workspace/_libraries/c_bindings/include/. staging-c3/native/include/
find /workspace/_libraries/c_bindings/build -maxdepth 1 -type f \( -name 'libcoolbox_c_bindings.*' -o -name 'coolbox_c_bindings.lib' -o -name 'coolbox_c_bindings.dll' \) -exec cp {} staging-c3/native/lib/ \;
cp -R staging-c3/native/lib/. .prebuilt/c3-native/lib/ 2>/dev/null || true

if command -v c3c >/dev/null 2>&1; then
  c3c compile-test /workspace/_libraries/c3_bindings/src/coolbox.c3 /workspace/_libraries/c3_bindings/tests/coolbox_test.c3 -I /workspace/_libraries/c_bindings/include -L /workspace/_libraries/c_bindings/build -l coolbox_c_bindings
else
  echo "C3 compiler not installed in local Linux CI image; packaging sources and native dependency bundle without smoke tests."
fi

tar -C staging-c3 -czf "$(package_tarball_path c3-bindings)" .
create_zip_from_dir staging-c3 "$(package_zip_path c3-bindings)"
finalize_release_assets

echo "Generated C3 purchase package for ${ref_name}"