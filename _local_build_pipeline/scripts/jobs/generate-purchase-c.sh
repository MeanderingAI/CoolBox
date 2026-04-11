#!/usr/bin/env bash
set -euo pipefail

cd /workspace

ref_name="$(printf '%s' "${GITHUB_REF_NAME:-local}")"
rm -rf _libraries/c_bindings/build release-assets staging-c
mkdir -p release-assets staging-c/include staging-c/lib

cmake -S _libraries/c_bindings -B _libraries/c_bindings/build -DBUILD_TESTING=ON
cmake --build _libraries/c_bindings/build --config Release
ctest --test-dir _libraries/c_bindings/build --output-on-failure

bash _libraries/c_bindings/generate_docs.sh
cp -R _libraries/c_bindings/include/. staging-c/include/
cp -R _libraries/c_bindings/target/. staging-c/
find _libraries/c_bindings/build -maxdepth 1 -type f \( -name 'libcoolbox_c_bindings.*' -o -name 'coolbox_c_bindings.lib' -o -name 'coolbox_c_bindings.dll' \) -exec cp {} staging-c/lib/ \;
tar -C staging-c -czf "release-assets/coolbox-c-bindings-linux-x86_64-${ref_name}.tar.gz" .
python3 -c 'import pathlib,sys,zipfile; s=pathlib.Path(sys.argv[1]); z=pathlib.Path(sys.argv[2]); a=zipfile.ZipFile(z,"w",compression=zipfile.ZIP_DEFLATED); [a.write(p,p.relative_to(s)) for p in sorted(s.rglob("*")) if p.is_file()]; a.close()' \
  staging-c "release-assets/coolbox-c-bindings-linux-x86_64-${ref_name}.zip"
