#!/usr/bin/env bash
set -euo pipefail

# shellcheck source=common.sh
source /workspace/_local_build_pipeline/scripts/jobs/common.sh

cd /workspace

rm -rf _libraries/c_bindings/build staging-c
stage_release_dir
mkdir -p staging-c/include staging-c/lib

cmake -S _libraries/c_bindings -B _libraries/c_bindings/build -DBUILD_TESTING=ON
cmake --build _libraries/c_bindings/build --config Release
ctest --test-dir _libraries/c_bindings/build --output-on-failure

bash _libraries/c_bindings/generate_docs.sh
cp -R _libraries/c_bindings/include/. staging-c/include/
cp -R _libraries/c_bindings/target/. staging-c/
find _libraries/c_bindings/build -maxdepth 1 -type f \( -name 'libcoolbox_c_bindings.*' -o -name 'coolbox_c_bindings.lib' -o -name 'coolbox_c_bindings.dll' \) -exec cp {} staging-c/lib/ \;
tar -C staging-c -czf "$(package_tarball_path c-bindings)" .
create_zip_from_dir staging-c "$(package_zip_path c-bindings)"
finalize_release_assets
