#!/usr/bin/env bash
set -euo pipefail

# shellcheck source=common.sh
source /workspace/_local_build_pipeline/scripts/jobs/common.sh

ensure_local_build
ensure_python_env
ref_name="$(default_ref_name)"
stage_release_dir

export COOLBOX_LIB_DIR=/workspace/build
export COOLBOX_PYTHON_FORCE_VENDOR_SOURCES=1

cd /workspace/_deliverables/libraries/bindings/python_bindings
rm -rf build dist ml_toolbox.egg-info vendor_include vendor_src
python setup.py build
python -m build --outdir ./dist

cd /workspace
rm -rf staging-python
mkdir -p staging-python
cp -R _deliverables/libraries/bindings/python_bindings/dist/. staging-python/ 2>/dev/null || true
find _deliverables/libraries/bindings/python_bindings -type f \( -name '*.so' -o -name '*.pyd' -o -name '*.dylib' -o -name '*.dll' \) -exec cp {} staging-python/ \;
tar -C staging-python -czf "$(package_tarball_path python-bindings)" .
create_zip_from_dir staging-python "$(package_zip_path python-bindings)"
finalize_release_assets
