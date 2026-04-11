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

cd /workspace/_libraries/python_bindings
rm -rf build dist ml_toolbox.egg-info vendor_include vendor_src
python setup.py build
python -m build --outdir ./dist

cd /workspace
rm -rf staging-python
mkdir -p staging-python
cp -R _libraries/python_bindings/dist/. staging-python/ 2>/dev/null || true
find _libraries/python_bindings -type f \( -name '*.so' -o -name '*.pyd' -o -name '*.dylib' -o -name '*.dll' \) -exec cp {} staging-python/ \;
tar -C staging-python -czf "release-assets/coolbox-python-bindings-linux-x86_64-${ref_name}.tar.gz" .
python3 -c 'import pathlib,sys,zipfile; s=pathlib.Path(sys.argv[1]); z=pathlib.Path(sys.argv[2]); a=zipfile.ZipFile(z,"w",compression=zipfile.ZIP_DEFLATED); [a.write(p,p.relative_to(s)) for p in sorted(s.rglob("*")) if p.is_file()]; a.close()' \
  staging-python "release-assets/coolbox-python-bindings-linux-x86_64-${ref_name}.zip"
