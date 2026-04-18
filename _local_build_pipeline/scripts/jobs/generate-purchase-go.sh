#!/usr/bin/env bash
set -euo pipefail

# shellcheck source=common.sh
source /workspace/_local_build_pipeline/scripts/jobs/common.sh

ensure_local_build
ref_name="$(default_ref_name)"
stage_release_dir

version_ge() {
	local current="$1"
	local required="$2"
	local first
	first="$(printf '%s\n%s\n' "$required" "$current" | sort -V | head -n 1)"
	[ "$first" = "$required" ]
}

ensure_go_toolchain() {
	local go_mod="/workspace/_libraries/go_bindings/go.mod"
	local required_version
	required_version="$(awk '/^go / { print $2; exit }' "$go_mod")"
	if [ -z "$required_version" ]; then
		echo "Unable to determine required Go version from $go_mod" >&2
		exit 1
	fi

	local current_version=""
	if command -v go >/dev/null 2>&1; then
		current_version="$(go env GOVERSION 2>/dev/null | sed 's/^go//')"
	fi

	if [ -n "$current_version" ] && version_ge "$current_version" "$required_version"; then
		echo "Using existing Go toolchain $current_version"
		return 0
	fi

	local go_arch
	case "$(uname -m)" in
		x86_64) go_arch="amd64" ;;
		aarch64|arm64) go_arch="arm64" ;;
		*)
			echo "Unsupported architecture for local Go bootstrap: $(uname -m)" >&2
			exit 1
			;;
		esac

	local download_version="$required_version"
	case "$download_version" in
		*.*.*) ;;
		*) download_version="${download_version}.0" ;;
		esac

	local install_root="/tmp/coolbox-go-toolchains"
	local install_dir="$install_root/go${download_version}"
	local archive="$install_root/go${download_version}.linux-${go_arch}.tar.gz"
	local url="https://go.dev/dl/go${download_version}.linux-${go_arch}.tar.gz"

	mkdir -p "$install_root"
	if [ ! -x "$install_dir/bin/go" ]; then
		echo "Installing Go ${download_version} for local purchase-go validation"
		curl -fL "$url" -o "$archive"
		rm -rf "$install_dir"
		mkdir -p "$install_dir"
		tar -xzf "$archive" -C "$install_dir" --strip-components=1
	fi

	export GOROOT="$install_dir"
	export PATH="$GOROOT/bin:$PATH"
	echo "Using bootstrapped Go toolchain: $(go version)"
}

ensure_go_toolchain

export COOLBOX_LIB_DIR=/workspace/build
export COOLBOX_C_BINDINGS_BUILD_DIR=/workspace/_libraries/c_bindings/build
export COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR=/workspace/_libraries/c_bindings/build/Release
export LD_LIBRARY_PATH="${COOLBOX_C_BINDINGS_BUILD_DIR}:${COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR}:${LD_LIBRARY_PATH:-}"
export DYLD_LIBRARY_PATH="${COOLBOX_C_BINDINGS_BUILD_DIR}:${COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR}:${DYLD_LIBRARY_PATH:-}"

candidates=(
	"${COOLBOX_C_BINDINGS_BUILD_DIR}/libcoolbox_c_bindings.so"
	"${COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR}/libcoolbox_c_bindings.so"
	"${COOLBOX_C_BINDINGS_BUILD_DIR}/libcoolbox_c_bindings.dylib"
	"${COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR}/libcoolbox_c_bindings.dylib"
)

found=""
for candidate in "${candidates[@]}"; do
	if [ -f "$candidate" ]; then
		found="$candidate"
		break
	fi
done

if [ -z "$found" ]; then
	echo "Expected native C bindings artifact was not found for Go tests." >&2
	echo "Contents of ${COOLBOX_C_BINDINGS_BUILD_DIR}:" >&2
	ls -la "${COOLBOX_C_BINDINGS_BUILD_DIR}" >&2 || true
	echo "Contents of ${COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR}:" >&2
	ls -la "${COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR}" >&2 || true
	exit 1
fi

mkdir -p build/test-logs
cd /workspace/_libraries/go_bindings
go test -count=1 -v ./... | tee /workspace/build/test-logs/go-bindings-linux-x86_64.log

cd /workspace
rm -rf staging-go
mkdir -p staging-go
cp -R _libraries/go_bindings/. staging-go/
rm -rf staging-go/.git staging-go/.github
tar -C staging-go -czf "$(package_tarball_path go-bindings)" .
create_zip_from_dir staging-go "$(package_zip_path go-bindings)"
finalize_release_assets
