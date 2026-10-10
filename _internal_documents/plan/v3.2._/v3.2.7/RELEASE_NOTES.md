# CoolBox v3.2.7 Release Notes

## Cross-Platform Python Source Distributions

Fixed Ubuntu and macOS wheel builds from isolated Python source distributions.
The PDE/SPDE binding previously included headers through repository-relative
paths such as:

```text
../../../groups/cool_car/MATH/pde_solver.hpp
```

Those paths are unavailable after the source archive is extracted into a
temporary build directory.

The Python package now ships package-local copies of:

- `pde_solver.hpp`;
- `spde.hpp`;
- `xarray.h`; and
- the required matrix headers.

The PDE/SPDE binding includes these files through `vendor_include`, making the
sdist independent of the surrounding CoolBox checkout.

## Manifest and Wheel Integrity

Removed obsolete `MANIFEST.in` patterns that referenced CoolBox directories
outside the Python package. Fresh source distributions no longer report that
the external MATRIX and deep-learning paths contain no matching files.

The purchase workflow now validates that:

- package headers contain no repository-relative `../../../groups/` includes;
- all required vendored MATH headers exist before compilation; and
- stale native extension files are removed before creating platform
  artifacts.

The final check prevents tracked Windows `.pyd` files and native extensions
from other Python or operating-system versions from leaking into macOS or
Linux wheels.

## Windows Native Build Scope

The Windows purchase job no longer builds the complete CoolBox solution with
tests, apps, and products enabled before packaging Python. That redundant
parallel build caused two targets to invoke vcpkg's `z-applocal` staging for
the same runtime files concurrently. The `plvhdl_lsp_test` staging command
then failed with Windows error 32 because another process held the file.

Windows packaging now performs only the existing focused native build for
`charts` and `wave_generator_utils`, with tests, binaries, and products
disabled. Required vcpkg and Bison path validation remains in that focused
step, and native target parallelism is capped at two jobs.

## LSP Package Publishing

Fixed the LSP package publisher's missing-artifact failure:

```text
Artifact not found for name: pl-lsp-binaries-linux-x86_64-v3.2.6
```

The image-generation job previously downloaded or locally built the LSP
binaries for Docker images but did not upload them for its dependent package
publishing job. It now uploads the verified Linux binaries under the exact
artifact name consumed by `publish_lsp_packages`.

Package staging no longer ignores missing `plang`, Rust, Java, or Python LSP
binaries. A missing executable now fails before an incomplete archive or
release can be created.

## Verification

- [x] Built a fresh Python source distribution.
- [x] Confirmed the archive contains the PDE, SPDE, xarray, and matrix
  headers.
- [x] Confirmed no repository-relative PDE/SPDE includes remain.
- [x] Confirmed the Python purchase workflow YAML passes linting.
- [x] Confirmed the Windows failure came from vcpkg `z-applocal` file
  contention in an unrelated LSP test target.
- [x] Removed the redundant full-solution Windows build from Python
  packaging.
- [x] Aligned the LSP artifact producer and consumer names in the purchase
  pipeline.
- [x] Confirmed `git diff --check` passes.
- [ ] Rerun Ubuntu, macOS, and Windows Python purchase jobs from a commit
  containing these changes.
