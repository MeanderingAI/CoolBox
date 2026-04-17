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
export LD_LIBRARY_PATH="${COOLBOX_C_BINDINGS_BUILD_DIR}:${COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR}:${LD_LIBRARY_PATH:-}"
export DYLD_LIBRARY_PATH="${COOLBOX_C_BINDINGS_BUILD_DIR}:${COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR}:${DYLD_LIBRARY_PATH:-}"

cmake -S _libraries/c_bindings -B _libraries/c_bindings/build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build _libraries/c_bindings/build --config Release

runtime_candidates=(
	"${COOLBOX_C_BINDINGS_BUILD_DIR}/libcoolbox_c_bindings.so"
	"${COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR}/libcoolbox_c_bindings.so"
	"${COOLBOX_C_BINDINGS_BUILD_DIR}/libcoolbox_c_bindings.dylib"
	"${COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR}/libcoolbox_c_bindings.dylib"
)

found_runtime=""
for candidate in "${runtime_candidates[@]}"; do
	if [ -f "${candidate}" ]; then
		found_runtime="${candidate}"
		break
	fi
done

if [ -z "${found_runtime}" ]; then
	echo "coolbox_c_bindings runtime artifact was not produced in expected locations." >&2
	printf '  %s\n' "${runtime_candidates[@]}" >&2
	ls -la "${COOLBOX_C_BINDINGS_BUILD_DIR}" >&2 || true
	ls -la "${COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR}" >&2 || true
	exit 1
fi

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
