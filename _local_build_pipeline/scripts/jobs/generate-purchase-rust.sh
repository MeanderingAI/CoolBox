#!/usr/bin/env bash
set -euo pipefail

# shellcheck source=common.sh
source /workspace/_local_build_pipeline/scripts/jobs/common.sh

ensure_local_build
ref_name="$(default_ref_name)"
stage_release_dir

export COOLBOX_LIB_DIR=/workspace/build
export COOLBOX_C_BINDINGS_BUILD_DIR=/workspace/_libraries/c_bindings/build
export COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR=/workspace/_libraries/c_bindings/build/Release

cmake -S _libraries/c_bindings -B _libraries/c_bindings/build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build _libraries/c_bindings/build --config Release

cargo build --manifest-path _libraries/rust_bindings/Cargo.toml --release
cargo test --manifest-path _libraries/rust_bindings/Cargo.toml --release

rm -rf staging-rust
mkdir -p staging-rust
cargo package --manifest-path _libraries/rust_bindings/Cargo.toml --allow-dirty --no-verify
cp target/package/*.crate "release-assets/$(package_stem rust-bindings).crate"
tar -xzf "release-assets/$(package_stem rust-bindings).crate" -C staging-rust
find staging-rust -exec touch -t 198001010000 {} +
tar -C staging-rust -czf "$(package_tarball_path rust-bindings)" .
create_zip_from_dir staging-rust "$(package_zip_path rust-bindings)"
finalize_release_assets
