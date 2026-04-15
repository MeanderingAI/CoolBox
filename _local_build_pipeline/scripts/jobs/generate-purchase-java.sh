#!/usr/bin/env bash
set -euo pipefail

# shellcheck source=common.sh
source /workspace/_local_build_pipeline/scripts/jobs/common.sh

ensure_local_build
ref_name="$(default_ref_name)"
stage_release_dir

rm -rf .prebuilt staging-java
mkdir -p .prebuilt/java-native staging-java

# Java bindings need the native C bindings/runtime assets, so build the local Linux variant first.
bash ./_local_build_pipeline/scripts/jobs/generate-purchase-c.sh

tar -xzf "release-assets/coolbox-c-bindings-linux-x86_64-${ref_name}.tar.gz" -C .prebuilt/java-native
find build/_libraries/backages/MISC -maxdepth 2 -type f \( -name '*.so' -o -name '*.a' \) -exec cp {} .prebuilt/java-native/lib/ \; 2>/dev/null || true

export COOLBOX_C_BINDINGS_DIR=/workspace/.prebuilt/java-native/lib
export LD_LIBRARY_PATH=/workspace/.prebuilt/java-native/lib:${LD_LIBRARY_PATH:-}

mvn \
  -f _libraries/java_bindings/pom.xml \
  -Dskip.c.bindings.build=true \
  -Dcoolbox.c.bindings.dir=/workspace/.prebuilt/java-native/lib \
  test package javadoc:javadoc

cp -R _libraries/java_bindings/target/. staging-java/
mkdir -p staging-java/native
cp -R .prebuilt/java-native/. staging-java/native/
tar -C staging-java -czf "$(package_tarball_path java-bindings)" .
create_zip_from_dir staging-java "$(package_zip_path java-bindings)"
finalize_release_assets