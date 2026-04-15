#!/usr/bin/env bash
set -euo pipefail

cd /workspace

ensure_local_build() {
  if [ ! -d build ] || [ ! -f build/CMakeCache.txt ]; then
    bash ./_local_build_pipeline/scripts/jobs/build-libs.sh
  fi
}

ensure_python_env() {
  if [ ! -d .venv ]; then
    python3 -m venv .venv
  fi
  # shellcheck disable=SC1091
  . .venv/bin/activate
  python -m pip install --upgrade pip setuptools wheel build pybind11 numpy
}

default_ref_name() {
  printf '%s' "${GITHUB_REF_NAME:-local}"
}

default_platform_tag() {
  printf '%s' "${COOLBOX_PLATFORM_TAG:-linux-x86_64}"
}

package_stem() {
  local package_name="$1"
  printf 'coolbox-%s-%s-%s' "${package_name}" "$(default_platform_tag)" "$(default_ref_name)"
}

package_tarball_path() {
  local package_name="$1"
  printf 'release-assets/%s.tar.gz' "$(package_stem "${package_name}")"
}

package_zip_path() {
  local package_name="$1"
  printf 'release-assets/%s.zip' "$(package_stem "${package_name}")"
}

create_zip_from_dir() {
  local source_dir="$1"
  local output_path="$2"
  python3 -c 'import pathlib,sys,zipfile; s=pathlib.Path(sys.argv[1]); z=pathlib.Path(sys.argv[2]); a=zipfile.ZipFile(z,"w",compression=zipfile.ZIP_DEFLATED); [a.write(p,p.relative_to(s)) for p in sorted(s.rglob("*")) if p.is_file()]; a.close()' \
    "${source_dir}" "${output_path}"
}

finalize_release_assets() {
  if [ ! -d release-assets ]; then
    return 0
  fi

  local manifest="release-assets/manifest.txt"
  local asset_names=()
  local asset_path

  shopt -s nullglob
  for asset_path in release-assets/*; do
    if [ -f "${asset_path}" ]; then
      case "${asset_path##*/}" in
        manifest.txt|SHA256SUMS)
          ;;
        *)
          asset_names+=("${asset_path##*/}")
          ;;
      esac
    fi
  done

  {
    printf 'schema=purchase-asset-manifest-v1\n'
    printf 'repository=%s\n' "${GITHUB_REPOSITORY:-local-workspace}"
    printf 'workflow=%s\n' "${GITHUB_WORKFLOW:-local-pipeline}"
    printf 'run_id=%s\n' "${GITHUB_RUN_ID:-local}"
    printf 'run_attempt=%s\n' "${GITHUB_RUN_ATTEMPT:-1}"
    printf 'ref=%s\n' "$(default_ref_name)"
    printf 'commit_sha=%s\n' "${GITHUB_SHA:-local}"
    printf 'platform=%s\n' "$(default_platform_tag)"
    printf 'asset_count=%s\n' "${#asset_names[@]}"
    for asset_name in "${asset_names[@]}"; do
      printf 'asset=%s\n' "${asset_name}"
    done
    printf 'generated_by=%s\n' "${0##*/}"
  } > "${manifest}"

  if command -v sha256sum >/dev/null 2>&1; then
    (cd release-assets && sha256sum * > SHA256SUMS)
  elif command -v shasum >/dev/null 2>&1; then
    (cd release-assets && shasum -a 256 * > SHA256SUMS)
  fi
}

stage_release_dir() {
  rm -rf release-assets
  mkdir -p release-assets
}
