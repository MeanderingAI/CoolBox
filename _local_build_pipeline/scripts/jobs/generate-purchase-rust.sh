#!/usr/bin/env bash
set -euo pipefail

# shellcheck source=common.sh
source /workspace/_local_build_pipeline/scripts/jobs/common.sh

ensure_local_build
ref_name="$(default_ref_name)"
stage_release_dir

export COOLBOX_LIB_DIR=/workspace/build

cargo build --manifest-path _libraries/rust_bindings/Cargo.toml --release
cargo test --manifest-path _libraries/rust_bindings/Cargo.toml --release

rm -rf staging-rust
mkdir -p staging-rust
cargo package --manifest-path _libraries/rust_bindings/Cargo.toml --allow-dirty --no-verify
cp target/package/*.crate "release-assets/coolbox-rust-bindings-${ref_name}.crate"
tar -xzf "release-assets/coolbox-rust-bindings-${ref_name}.crate" -C staging-rust
find staging-rust -exec touch -t 198001010000 {} +
tar -C staging-rust -czf "release-assets/coolbox-rust-bindings-${ref_name}.tar.gz" .
python3 -c 'import pathlib,sys,zipfile; s=pathlib.Path(sys.argv[1]); z=pathlib.Path(sys.argv[2]); a=zipfile.ZipFile(z,"w",compression=zipfile.ZIP_DEFLATED); [a.write(p,p.relative_to(s)) for p in sorted(s.rglob("*")) if p.is_file()]; a.close()' \
  staging-rust "release-assets/coolbox-rust-bindings-${ref_name}.zip"
