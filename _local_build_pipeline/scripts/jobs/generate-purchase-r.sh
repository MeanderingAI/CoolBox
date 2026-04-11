#!/usr/bin/env bash
set -euo pipefail

# shellcheck source=common.sh
source /workspace/_local_build_pipeline/scripts/jobs/common.sh

ensure_local_build
ref_name="$(default_ref_name)"
stage_release_dir

export COOLBOX_LIB_DIR=/workspace/build
export R_LIBS_USER=/workspace/.r-library
mkdir -p "${R_LIBS_USER}"
Rscript -e "lib <- Sys.getenv('R_LIBS_USER'); dir.create(lib, recursive = TRUE, showWarnings = FALSE); .libPaths(c(lib, .libPaths())); install.packages('Rcpp', repos = 'https://cloud.r-project.org', lib = lib)"

rm -rf staging-r
mkdir -p staging-r
find _libraries/r_bindings/coolboxr/src -maxdepth 1 -type f \( -name '*.o' -o -name '*.so' -o -name '*.dll' -o -name '*.dylib' -o -name '*.a' -o -name '*.lib' \) -delete
R CMD build _libraries/r_bindings/coolboxr
mv coolboxr_*.tar.gz "release-assets/coolbox-r-bindings-linux-x86_64-${ref_name}.tar.gz"
tar -xzf "release-assets/coolbox-r-bindings-linux-x86_64-${ref_name}.tar.gz" -C staging-r
python3 -c 'import pathlib,sys,zipfile; s=pathlib.Path(sys.argv[1]); z=pathlib.Path(sys.argv[2]); a=zipfile.ZipFile(z,"w",compression=zipfile.ZIP_DEFLATED); [a.write(p,p.relative_to(s)) for p in sorted(s.rglob("*")) if p.is_file()]; a.close()' \
  staging-r "release-assets/coolbox-r-bindings-linux-x86_64-${ref_name}.zip"
